<#
.SYNOPSIS
    Generate sdk/x4_md_events.h from runtime dump data + common.xsd.

.DESCRIPTION
    Reads:
      - reference/event_type_ids.csv  (from class_dump: type IDs, RTTI names, short names)
      - reference/event_layouts.csv   (from event_probe: field offsets + sizes)
      - reference/game/libraries/common.xsd (XSD event parameter documentation)

    Generates:
      - sdk/x4_md_events.h  (typed subscription functions with auto-extracted structs)

    Event type IDs and struct layouts shift between game builds.
    Re-run class_dump + event_probe in-game after each game update.

.PARAMETER GameVersion
    Version string for the header comment. Auto-detected from reference/game/VERSION.

.EXAMPLE
    .\scripts\generate_event_type_ids.ps1
#>
param(
    [string]$GameVersion
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent

$csvFile     = Join-Path $repoRoot 'reference\event_type_ids.csv'
$layoutFile  = Join-Path $repoRoot 'reference\event_layouts.csv'
$xsdFile     = Join-Path $repoRoot 'reference\game\libraries\common.xsd'
$versionFile = Join-Path $repoRoot 'reference\game\VERSION'
$outHeader   = Join-Path $repoRoot 'sdk\x4_md_events.h'

if (-not (Test-Path $csvFile)) {
    Write-Error "reference/event_type_ids.csv not found. Run class_dump extension in-game first."
    exit 1
}

if (-not $GameVersion) {
    $GameVersion = if (Test-Path $versionFile) { (Get-Content $versionFile -Raw).Trim() } else { 'unknown' }
}

# --- Parse CSV ---
$rows = Import-Csv $csvFile

# --- Parse XSD for event documentation (optional) ---
$eventDocs = @{}
if (Test-Path $xsdFile) {
    $xsdContent = Get-Content $xsdFile -Raw
    # Match: <xs:element name="event_*">...<xs:documentation>DOCS</xs:documentation>
    $pattern = '(?s)<xs:element\s+name="(event_[^"]+)"[^>]*>.*?<xs:documentation>\s*(.*?)\s*</xs:documentation>'
    $matches = [regex]::Matches($xsdContent, $pattern)
    foreach ($m in $matches) {
        $eventName = $m.Groups[1].Value
        $doc = $m.Groups[2].Value -replace '\s+', ' '
        $eventDocs[$eventName] = $doc.Trim()
    }
    Write-Host "  Parsed $($eventDocs.Count) event docs from common.xsd"
}

# --- Match short_names to XSD event names ---
# Normalize XSD names by stripping "event_" prefix and underscores, then match
# against engine short_names. Also strips common prefixes (object, player, etc.)
# and applies synonyms (killed/destroyed) for vocabulary mismatches.
function Normalize-XsdName {
    param([string]$xsd)
    $n = $xsd
    if ($n.StartsWith("event_")) { $n = $n.Substring(6) }
    return $n -replace '_', ''
}

# Build XSD short-form → full XSD name lookup (multiple normalized forms per XSD name)
$xsdByShort = @{}
foreach ($xsdName in $eventDocs.Keys) {
    $short = Normalize-XsdName $xsdName
    $xsdByShort[$short] = $xsdName
    # Also strip common prefixes to create additional lookup keys
    foreach ($prefix in @('object', 'contained', 'containedsector', 'containedentity',
                          'player', 'playerowned', 'playerownedobject', 'god', 'faction',
                          'ui', 'entity')) {
        if ($short.StartsWith($prefix) -and $short.Length -gt $prefix.Length) {
            $stripped = $short.Substring($prefix.Length)
            # Only add if not already mapped (prefer more specific match)
            if (-not $xsdByShort.ContainsKey($stripped)) {
                $xsdByShort[$stripped] = $xsdName
            }
        }
    }
}

# Build entries sorted by ID with three-way matching
$matchedCount = 0
$entries = foreach ($row in $rows | Sort-Object { [int]$_.id }) {
    $rttiName = $row.rtti_name
    $shortName = $row.short_name
    $xsdName = $null
    $doc = $null

    if ($shortName) {
        $synonyms = @{
            'killed' = 'destroyed'
            'playerownedobjectwaskilled' = 'playerowneddestroyed'
        }
        $namesToTry = @($shortName)
        if ($synonyms.ContainsKey($shortName)) {
            $namesToTry += $synonyms[$shortName]
        }

        foreach ($tryName in $namesToTry) {
            # Direct lookup: short_name matches normalized XSD (includes prefix-stripped variants)
            if ($xsdByShort.ContainsKey($tryName)) {
                $xsdName = $xsdByShort[$tryName]
                break
            }
            # Add prefix to short_name and try
            foreach ($prefix in @('object', 'contained', 'containedsector', 'containedentity',
                                  'player', 'playerowned', 'playerownedobject', 'god', 'faction',
                                  'ui', 'entity')) {
                $candidate = $prefix + $tryName
                if ($xsdByShort.ContainsKey($candidate)) {
                    $xsdName = $xsdByShort[$candidate]
                    break
                }
            }
            if ($xsdName) { break }
        }

        if ($xsdName -and $eventDocs.ContainsKey($xsdName)) {
            $doc = $eventDocs[$xsdName]
            $matchedCount++
        }
    }

    # Use rtti_name for the enum entry; fall back to short_name if RTTI missing
    $enumName = if ($rttiName) { $rttiName } elseif ($shortName) {
        # PascalCase the short_name: "killed" → "Killed", append "Event"
        ($shortName.Substring(0,1).ToUpper() + $shortName.Substring(1)) + "Event"
    } else { "Unknown_$($row.id)" }

    [PSCustomObject]@{
        Id        = [int]$row.id
        Name      = $enumName
        ShortName = $shortName
        XsdName   = $xsdName
        Doc       = $doc
    }
}

# --- Parse event_layouts.csv (optional — from event_probe extension) ---
$layouts = @{}  # type_id → { alloc_size, fields = @( @{offset, size} ) }
if (Test-Path $layoutFile) {
    $layoutRows = Import-Csv $layoutFile
    foreach ($row in $layoutRows) {
        $id = [int]$row.id
        $fields = @()
        if ($row.fields -and $row.fields.Trim() -ne '') {
            foreach ($f in $row.fields -split ';') {
                $parts = $f -split ':'
                if ($parts.Count -eq 2) {
                    $fields += [PSCustomObject]@{
                        Offset = [int]$parts[0]
                        Size   = [int]$parts[1]
                    }
                }
            }
        }
        $layouts[$id] = [PSCustomObject]@{
            AllocSize = [int]$row.alloc_size
            Fields    = $fields
        }
    }
    Write-Host "  Parsed $($layouts.Count) event layouts from event_layouts.csv"
} else {
    Write-Host "  event_layouts.csv not found - skipping struct generation"
}

# --- XSD parameter parsing ---
function Parse-XsdParams {
    param([string]$doc)
    $params = @()
    # Match: "param = description" or "param2 = [list]" or "object = description"
    # Handle bracketed lists: param3 = [val1, val2, val3]
    $regex = '(object|param\d*)\s*=\s*(\[[^\]]+\]|[^,\)]+)'
    $ms = [regex]::Matches($doc, $regex)
    foreach ($m in $ms) {
        $paramKey = $m.Groups[1].Value.Trim()
        $paramDesc = $m.Groups[2].Value.Trim()

        # Check for list syntax: "[value1, value2, value3]" -> expand into separate params
        if ($paramDesc -match '^\[(.+)\]') {
            $listItems = $Matches[1] -split '\s*,\s*'
            foreach ($item in $listItems) {
                $fieldName = ($item -replace '[^a-zA-Z0-9_ ]', '' -replace '\s+', '_').ToLower().Trim('_')
                if ($fieldName.Length -gt 30) { $fieldName = $fieldName.Substring(0, 30).TrimEnd('_') }
                if (-not $fieldName) { $fieldName = $paramKey }
                $params += [PSCustomObject]@{
                    Key       = $paramKey
                    FieldName = $fieldName
                    Desc      = $item.Trim()
                }
            }
        } else {
            $fieldName = ($paramDesc -replace '[^a-zA-Z0-9_ ]', '' -replace '\s+', '_').ToLower().Trim('_')
            if ($fieldName.Length -gt 30) { $fieldName = $fieldName.Substring(0, 30).TrimEnd('_') }
            if (-not $fieldName) { $fieldName = $paramKey }
            $params += [PSCustomObject]@{
                Key       = $paramKey
                FieldName = $fieldName
                Desc      = $paramDesc
            }
        }
    }
    return $params
}

# Map field size to C type
function Size-ToCType {
    param([int]$size)
    if ($size -eq 1) { return 'uint8_t' }
    if ($size -eq 2) { return 'uint16_t' }
    if ($size -eq 4) { return 'uint32_t' }
    if ($size -eq 8) { return 'uint64_t' }
    return "uint8_t[$size]"
}

# --- Generate x4_md_events.h ---
$hdr = [System.Collections.Generic.List[string]]::new()
$hdr.Add("// ==========================================================================")
$hdr.Add("// x4_md_events.h - Typed MD Event Subscription API")
$hdr.Add("// ==========================================================================")
$hdr.Add("// Auto-generated from event_type_ids.csv + event_layouts.csv + common.xsd")
$hdr.Add("// Game version: $GameVersion")
$hdr.Add("//")
$hdr.Add("// Usage:")
$hdr.Add("//   x4n::md::on_sector_changed_owner_before([](const x4n::md::SectorChangedOwnerData& e) {")
$hdr.Add("//       // e.new_owner_faction, e.sector_changing_ownership")
$hdr.Add("//   });")
$hdr.Add("//")
$hdr.Add("// DO NOT EDIT - regenerate with: .\scripts\generate_event_type_ids.ps1")
$hdr.Add("// ==========================================================================")
$hdr.Add("#pragma once")
$hdr.Add("#include `"x4n_events.h`"")
$hdr.Add("#include <cstdint>")
$hdr.Add("")
$hdr.Add("namespace x4n::md")
$hdr.Add("{")
$hdr.Add("")
$hdr.Add("    namespace detail")
$hdr.Add("    {")
$hdr.Add("        template<typename T>")
$hdr.Add("        void trampoline(const char*, void* data, void* ud) {")
$hdr.Add("            auto* ev = static_cast<const X4MdEvent*>(data);")
$hdr.Add("            auto d = T::from(ev);")
$hdr.Add("            reinterpret_cast<void(*)(const T&)>(ud)(d);")
$hdr.Add("        }")
$hdr.Add("")
$hdr.Add('        // Internal subscribe helpers. Use the typed on_*_before/after functions below.')
$hdr.Add('        using MdSubscribeFn = int(*)(uint32_t, X4NativeEventCallback, void*, void*);')
$hdr.Add('        inline int subscribe_before(uint32_t type_id, X4NativeEventCallback cb, void* ud) {')
$hdr.Add('            auto fn = reinterpret_cast<MdSubscribeFn>(x4n::detail::g_api->md_subscribe_before);')
$hdr.Add('            return fn ? fn(type_id, cb, ud, x4n::detail::g_api) : -1;')
$hdr.Add('        }')
$hdr.Add('        inline int subscribe_after(uint32_t type_id, X4NativeEventCallback cb, void* ud) {')
$hdr.Add('            auto fn = reinterpret_cast<MdSubscribeFn>(x4n::detail::g_api->md_subscribe_after);')
$hdr.Add('            return fn ? fn(type_id, cb, ud, x4n::detail::g_api) : -1;')
$hdr.Add('        }')
$hdr.Add("    } // namespace detail")
$hdr.Add("")
$hdr.Add("")

# Convert PascalCase to snake_case: "SectorChangedOwnerEvent" -> "sector_changed_owner"
function To-SnakeCase {
    param([string]$name)
    if ($name.EndsWith("Event")) { $name = $name.Substring(0, $name.Length - 5) }
    $snake = [regex]::Replace($name, '(?<!^)([A-Z])', '_$1').ToLower()
    return $snake
}

$funcCount = 0
foreach ($e in $entries) {
    if (-not $e.Doc) { continue }  # Only events with XSD documentation
    if (-not $layouts.ContainsKey($e.Id)) { continue }
    $layout = $layouts[$e.Id]

    $xsdParams = @()
    if ($e.Doc) { $xsdParams = Parse-XsdParams $e.Doc }

    # Build field list
    $fieldDefs = @()
    $fieldIdx = 0
    $usedNames2 = @{}
    $payloadParams = @($xsdParams | Where-Object { $_.Key -ne 'object' })
    foreach ($f in $layout.Fields) {
        $ctype = Size-ToCType $f.Size
        $fieldName = "field_$($f.Offset.ToString('x2'))"
        if ($fieldIdx -lt $payloadParams.Count) {
            $fieldName = $payloadParams[$fieldIdx].FieldName
        }
        if ($usedNames2.ContainsKey($fieldName)) {
            $usedNames2[$fieldName]++
            $fieldName = "${fieldName}_$($usedNames2[$fieldName])"
        } else {
            $usedNames2[$fieldName] = 1
        }
        if ($fieldName -match '^field_[0-9a-f]+$') {
            $fieldIdx++
            continue
        }
        $fieldDefs += [PSCustomObject]@{
            CType = $ctype; FieldName = $fieldName; Offset = $f.Offset
        }
        $fieldIdx++
    }

    $snakeName = To-SnakeCase $e.Name
    $structName = "$($e.Name -replace 'Event$','')Data"

    # Doc comment
    if ($e.XsdName -and $e.Doc) {
        $truncDoc = if ($e.Doc.Length -gt 100) { $e.Doc.Substring(0, 97) + ".." } else { $e.Doc }
        $hdr.Add("    /// $truncDoc")
    }

    # Struct with from() — always includes source_id + timestamp from X4MdEvent
    $hdr.Add("    struct $structName {")
    $hdr.Add("        uint64_t source_id;        // Event source entity (X4MdEvent)")
    $hdr.Add("        double   timestamp;         // Game time (X4MdEvent)")
    foreach ($fd in $fieldDefs) {
        $hdr.Add("        $($fd.CType) $($fd.FieldName);")
    }
    if ($fieldDefs.Count -gt 0) {
        $hdr.Add("")
        $hdr.Add("        static $structName from(const X4MdEvent* ev) {")
        $hdr.Add("            auto* p = static_cast<const uint8_t*>(ev->raw_event);")
        $hdr.Add("            return {")
        $hdr.Add("                ev->source_id,")
        $hdr.Add("                ev->timestamp,")
        for ($fi = 0; $fi -lt $fieldDefs.Count; $fi++) {
            $fd = $fieldDefs[$fi]
            $comma = if ($fi -lt $fieldDefs.Count - 1) { ',' } else { '' }
            $hdr.Add("                *reinterpret_cast<const $($fd.CType)*>(p + 0x$($fd.Offset.ToString('X2')))$comma")
        }
        $hdr.Add("            };")
        $hdr.Add("        }")
    } else {
        $hdr.Add("        static $structName from(const X4MdEvent* ev) { return { ev->source_id, ev->timestamp }; }")
    }
    $hdr.Add("    };")
    $hdr.Add("")

    # on_*_before / on_*_after — delegate to local detail::subscribe_before/after
    $hdr.Add("    inline int on_${snakeName}_before(void(*cb)(const ${structName}&)) {")
    $hdr.Add("        return detail::subscribe_before($($e.Id), detail::trampoline<${structName}>,")
    $hdr.Add("            reinterpret_cast<void*>(cb));")
    $hdr.Add("    }")
    $hdr.Add("")
    $hdr.Add("    inline int on_${snakeName}_after(void(*cb)(const ${structName}&)) {")
    $hdr.Add("        return detail::subscribe_after($($e.Id), detail::trampoline<${structName}>,")
    $hdr.Add("            reinterpret_cast<void*>(cb));")
    $hdr.Add("    }")
    $hdr.Add("")
    $funcCount++
}

$hdr.Add("")
$hdr.Add("} // namespace x4n::md")
$hdr.Add("")

$hdrContent = $hdr -join "`n"
Set-Content -Path $outHeader -Value $hdrContent -Encoding UTF8 -NoNewline
Write-Host "  Generated $outHeader ($funcCount event functions)"

Write-Host "  Processed $($entries.Count) event types ($matchedCount with XSD docs)"
Write-Host "  $matchedCount event types matched to XSD docs (of $($eventDocs.Count) XSD events)"
$unmatchedXsd = $eventDocs.Keys | Where-Object { $_ -notin ($entries | Where-Object { $_.XsdName } | ForEach-Object { $_.XsdName }) }
if ($unmatchedXsd.Count -gt 0) {
    Write-Host "  $($unmatchedXsd.Count) XSD events not matched to any type ID (may be MD-only or obsolete)"
}
