<#
.SYNOPSIS
    Generate game data lookup tables from X4 reference files.

.DESCRIPTION
    Parses reference/game/libraries/ XML files and sdk/x4_manual_types.h to produce:
      - extension/ui/x4n_game_data.lua -- Lua room macro->type tables + lookup functions

    Reads the authoritative X4RoomType enum from x4_manual_types.h (IDA-confirmed).
    Parses rooms.xml + roomgroups.xml (base + DLC) for macro->roomtype mappings.
    Parses common.xsd for drift detection against known roomtypes.

    Designed to run as part of the update_references.ps1 pipeline (step 6),
    or standalone after updating reference data.

.EXAMPLE
    .\scripts\generate_game_data.ps1
#>
param(
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path $PSScriptRoot -Parent
$RefBase   = Join-Path $repoRoot "reference\game\libraries"
$ExtDir    = Join-Path $repoRoot "extension\ui"
$OutLua    = Join-Path $ExtDir "x4n_game_data.lua"
$ManualTypesFile = Join-Path $repoRoot "sdk\x4_manual_types.h"

# Room tag -> roomtype mapping overrides.
# Most rooms.xml tags map 1:1 to the X4RoomType enum. These are the exceptions.
$TAG_TO_ROOMTYPE = @{
    office     = "manager"    # office_* rooms are manager's offices
    bosotank   = "manager"    # boron office variant
    throneroom = "bar"        # boron throne room, functionally a bar
}

$warnings = @()

function Write-Status($msg) {
    if (-not $Quiet) { Write-Host $msg }
}

# ---------------------------------------------------------------------------
# Step 1: Parse X4RoomType enum from x4_manual_types.h (single source of truth)
# ---------------------------------------------------------------------------
Write-Status "Parsing X4RoomType enum from x4_manual_types.h..."

if (-not (Test-Path $ManualTypesFile)) {
    Write-Error "sdk/x4_manual_types.h not found. This file is hand-authored -- see docs/rev/WALKABLE_INTERIORS.md."
    exit 1
}

$manualContent = Get-Content $ManualTypesFile
$roomtypeEnum = [ordered]@{}  # lowercase_name -> index
$inEnum = $false

foreach ($line in $manualContent) {
    if ($line -match 'typedef\s+enum\s+X4RoomType') {
        $inEnum = $true
        continue
    }
    if ($inEnum -and $line -match '^\s*}\s*X4RoomType\s*;') {
        break
    }
    if ($inEnum -and $line -match 'X4_ROOMTYPE_(\w+)\s*=\s*(\d+)') {
        $name = $Matches[1].ToLower()
        $idx  = [int]$Matches[2]
        $roomtypeEnum[$name] = $idx
    }
}

if ($roomtypeEnum.Count -eq 0) {
    Write-Error "Failed to parse X4RoomType enum from x4_manual_types.h"
    exit 1
}

Write-Status "  Parsed $($roomtypeEnum.Count) roomtype entries"

# ---------------------------------------------------------------------------
# Step 2: Parse rooms.xml + DLC rooms -- build room_id -> room_type_tag
# ---------------------------------------------------------------------------
Write-Status "Parsing rooms.xml files..."

$roomFiles = @(
    (Join-Path $RefBase "rooms.xml")
)
$dlcRoomFiles = Get-ChildItem -Path (Join-Path $repoRoot "reference\game\extensions") `
    -Filter "rooms.xml" -Recurse -ErrorAction SilentlyContinue
foreach ($f in $dlcRoomFiles) { $roomFiles += $f.FullName }

$roomDefs = @{}

foreach ($file in $roomFiles) {
    if (-not (Test-Path $file)) { continue }
    $xml = [xml](Get-Content $file -Raw)

    foreach ($room in $xml.rooms.room) {
        $id    = $room.id
        $group = $room.group
        $tags  = $room.category.tags

        $tagList = @()
        if ($tags -match '^\[(.+)\]$') {
            $tagList = $Matches[1] -split ',\s*'
        } else {
            $tagList = @($tags.Trim())
        }

        $isCorridor = $tagList -contains "corridor"
        $typeTag = $null
        if (-not $isCorridor) {
            $modifiers = @("room", "generic", "shady")
            foreach ($t in $tagList) {
                if ($modifiers -notcontains $t) {
                    $typeTag = $t
                    break
                }
            }
        }

        $roomDefs[$id] = @{
            group       = $group
            type_tag    = $typeTag
            is_corridor = $isCorridor
        }
    }
}

Write-Status "  Found $($roomDefs.Count) room definitions"

# ---------------------------------------------------------------------------
# Step 3: Parse roomgroups.xml + DLC -- build group_name -> [macro, ...]
# ---------------------------------------------------------------------------
Write-Status "Parsing roomgroups.xml files..."

$groupFiles = @(
    (Join-Path $RefBase "roomgroups.xml")
)
$dlcGroupFiles = Get-ChildItem -Path (Join-Path $repoRoot "reference\game\extensions") `
    -Filter "roomgroups.xml" -Recurse -ErrorAction SilentlyContinue
foreach ($f in $dlcGroupFiles) { $groupFiles += $f.FullName }

$roomGroups = @{}

foreach ($file in $groupFiles) {
    if (-not (Test-Path $file)) { continue }
    $xml = [xml](Get-Content $file -Raw)
    foreach ($group in $xml.groups.group) {
        $name = $group.name
        if (-not $roomGroups.ContainsKey($name)) {
            $roomGroups[$name] = @()
        }
        foreach ($sel in $group.select) {
            $macro = $sel.macro
            if ($macro -and ($roomGroups[$name] -notcontains $macro)) {
                $roomGroups[$name] += $macro
            }
        }
    }
}

Write-Status "  Found $($roomGroups.Count) room groups"

# ---------------------------------------------------------------------------
# Step 4: Cross-reference -- build macro -> roomtype
# ---------------------------------------------------------------------------
Write-Status "Building macro -> roomtype mappings..."

$macroToType     = [ordered]@{}
$corridorMacros  = [ordered]@{}
$macroTypes      = @{}

foreach ($roomId in $roomDefs.Keys) {
    $def = $roomDefs[$roomId]
    $groupName = $def.group
    if (-not $roomGroups.ContainsKey($groupName)) { continue }
    $macros = $roomGroups[$groupName]

    if ($def.is_corridor) {
        foreach ($m in $macros) {
            $corridorMacros[$m] = $true
        }
    } else {
        $rawTag = $def.type_tag
        if (-not $rawTag) { continue }

        if ($TAG_TO_ROOMTYPE.ContainsKey($rawTag)) {
            $roomtype = $TAG_TO_ROOMTYPE[$rawTag]
        } else {
            $roomtype = $rawTag
        }

        # Validate roomtype exists in the enum
        if (-not $roomtypeEnum.Contains($roomtype)) {
            $warnings += "Room tag '$rawTag' maps to roomtype '$roomtype' which is not in X4RoomType enum"
        }

        foreach ($m in $macros) {
            if (-not $macroTypes.ContainsKey($m)) {
                $macroTypes[$m] = @()
            }
            if ($macroTypes[$m] -notcontains $roomtype) {
                $macroTypes[$m] += $roomtype
            }

            if (-not $macroToType.Contains($m)) {
                $macroToType[$m] = $roomtype
            } elseif ($macroToType[$m] -eq "factionrep" -and $roomtype -ne "factionrep") {
                $macroToType[$m] = $roomtype
            }
        }
    }
}

foreach ($m in $macroTypes.Keys) {
    if ($macroTypes[$m].Count -gt 1) {
        $types = $macroTypes[$m] -join ", "
        Write-Status "  NOTE: $m used as multiple types: $types (using: $($macroToType[$m]))"
    }
}

Write-Status "  $($macroToType.Count) room macros, $($corridorMacros.Count) corridor macros"

# ---------------------------------------------------------------------------
# Step 5: Parse common.xsd -- drift detection
# ---------------------------------------------------------------------------
Write-Status "Parsing common.xsd for roomtype drift detection..."

$xsdFile = Join-Path $RefBase "common.xsd"
if (Test-Path $xsdFile) {
    $xsdContent = Get-Content $xsdFile -Raw
    $xsdRoomTypes = @()
    if ($xsdContent -match '(?s)<xs:simpleType name="roomtypelookup">(.+?)</xs:simpleType>') {
        $block = $Matches[1]
        $enumMatches = [regex]::Matches($block, 'value="([^"]+)"')
        foreach ($em in $enumMatches) {
            $xsdRoomTypes += $em.Groups[1].Value
        }
    }

    foreach ($xt in $xsdRoomTypes) {
        if (-not $roomtypeEnum.Contains($xt)) {
            $warnings += "XSD roomtype '$xt' not in X4RoomType enum -- may need IDA analysis"
        }
    }
}

# ---------------------------------------------------------------------------
# Step 6: Generate Lua output
# ---------------------------------------------------------------------------
Write-Status "Generating $OutLua..."

if (-not (Test-Path $ExtDir)) { New-Item -Path $ExtDir -ItemType Directory -Force | Out-Null }

$luaLines = @()
$luaLines += "-- =========================================================================="
$luaLines += "-- x4n_game_data.lua -- Game Data Lookup Tables"
$luaLines += "-- =========================================================================="
$luaLines += "-- Auto-generated by scripts/generate_game_data.ps1"
$luaLines += "-- Source: sdk/x4_manual_types.h, reference/game/libraries/"
$luaLines += "--"
$luaLines += "-- DO NOT EDIT -- regenerate with: .\scripts\generate_game_data.ps1"
$luaLines += "-- =========================================================================="
$luaLines += ""
$luaLines += "-- Room macro name -> roomtype string."
$luaLines += "-- When a macro is used by multiple room definitions (e.g. managersoffice as"
$luaLines += '-- both "manager" and "factionrep"), the primary (non-factionrep) type is listed.'
$luaLines += "x4n_ROOM_MACRO_TO_TYPE = {"

$sortedMacros = $macroToType.Keys | Sort-Object
foreach ($m in $sortedMacros) {
    $t = $macroToType[$m]
    $luaLines += "    [`"$m`"] = `"$t`","
}
$luaLines += "}"
$luaLines += ""
$luaLines += "-- Set of corridor macros."
$luaLines += "x4n_CORRIDOR_MACROS = {"

$sortedCorridors = $corridorMacros.Keys | Sort-Object
foreach ($c in $sortedCorridors) {
    $luaLines += "    [`"$c`"] = true,"
}
$luaLines += "}"
$luaLines += ""
$luaLines += "-- Roomtype -> enum index (from X4RoomType in x4_manual_types.h)."
$luaLines += "x4n_ROOMTYPE_INDEX = {"
foreach ($key in $roomtypeEnum.Keys) {
    $val = $roomtypeEnum[$key]
    $luaLines += "    $key = $val,"
}
$luaLines += "}"
$luaLines += ""
$luaLines += "-- Returns true if the macro is a known corridor."
$luaLines += "function x4n_is_corridor(macro)"
$luaLines += "    return x4n_CORRIDOR_MACROS[macro] == true"
$luaLines += "end"
$luaLines += ""
$luaLines += "-- Returns roomtype_string, roomtype_index or nil, -1 if unknown."
$luaLines += "function x4n_lookup_room(macro)"
$luaLines += "    local rtype = x4n_ROOM_MACRO_TO_TYPE[macro]"
$luaLines += "    if rtype then"
$luaLines += "        return rtype, (x4n_ROOMTYPE_INDEX[rtype] or -1)"
$luaLines += "    end"
$luaLines += "    return nil, -1"
$luaLines += "end"

$luaContent = ($luaLines -join "`n") + "`n"
[System.IO.File]::WriteAllText($OutLua, $luaContent, [System.Text.UTF8Encoding]::new($false))
Write-Status "  Written: $OutLua"

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Write-Host ""
if ($warnings.Count -gt 0) {
    Write-Host "WARNINGS ($($warnings.Count)):" -ForegroundColor Yellow
    foreach ($w in $warnings) { Write-Host "  WARNING: $w" -ForegroundColor Yellow }
}
Write-Host "generate_game_data complete." -ForegroundColor Green
Write-Host "  Roomtype enum:   $($roomtypeEnum.Count) entries (from x4_manual_types.h)"
Write-Host "  Room macros:     $($macroToType.Count)"
Write-Host "  Corridor macros: $($corridorMacros.Count)"
