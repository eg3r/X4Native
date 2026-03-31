<#
.SYNOPSIS
    Generate x4_game_class_ids.inc from class_ids.csv.

.DESCRIPTION
    Reads reference/class_ids.csv (produced by the class_dump extension) and generates
    sdk/x4_game_class_ids.inc containing:
      - x4n::GameClass enum class (PascalCase, e.g. Station, ShipXs, WalkableModule)
      - X4_CLASS_* preprocessor aliases for C compatibility and legacy code

    Class IDs are NOT stable across game builds. This script regenerates per build.

.PARAMETER GameVersion
    Version string for the generated header comment. Auto-detected from reference/game/VERSION.

.EXAMPLE
    .\scripts\generate_class_ids.ps1
#>
param(
    [string]$GameVersion
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent

$csvFile     = Join-Path $repoRoot 'reference\class_ids.csv'
$versionFile = Join-Path $repoRoot 'reference\game\VERSION'
$outFile     = Join-Path $repoRoot 'sdk\x4_game_class_ids.inc'

if (-not (Test-Path $csvFile)) {
    Write-Error "reference/class_ids.csv not found. Run class_dump extension in-game first."
    exit 1
}

if (-not $GameVersion) {
    $GameVersion = if (Test-Path $versionFile) { (Get-Content $versionFile -Raw).Trim() } else { 'unknown' }
}

$rows = Import-Csv $csvFile
$maxId = ($rows | Measure-Object -Property id -Maximum).Maximum
$sentinel = [int]$maxId + 1

# Convert snake_case to PascalCase: "ship_xs" -> "ShipXs", "walkablemodule" -> "Walkablemodule"
function ConvertTo-PascalCase {
    param([string]$name)
    $parts = $name -split '_'
    $result = ''
    foreach ($part in $parts) {
        if ($part.Length -gt 0) {
            $result += $part.Substring(0,1).ToUpper() + $part.Substring(1)
        }
    }
    return $result
}

# Build entries: [ { id, raw_name, pascal_name, upper_name } ]
$entries = foreach ($row in $rows | Sort-Object { [int]$_.id }) {
    [PSCustomObject]@{
        Id         = [int]$row.id
        RawName    = $row.name
        PascalName = ConvertTo-PascalCase $row.name
        UpperName  = $row.name.ToUpper()
    }
}

# Compute padding widths
$maxPascalLen = ($entries | ForEach-Object { $_.PascalName.Length } | Measure-Object -Maximum).Maximum

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("// ==========================================================================")
$lines.Add("// x4_game_class_ids.inc - X4 Entity Class IDs")
$lines.Add("// ==========================================================================")
$lines.Add("// Auto-generated from class_ids.csv (game version: $GameVersion).")
$lines.Add("// $($entries.Count) classes, sentinel = $sentinel.")
$lines.Add("//")
$lines.Add("// Usage:")
$lines.Add("//   x4n::GameClass cls = x4n::entity::get_class_id(comp);")
$lines.Add("//   if (cls == x4n::GameClass::Station) { ... }")
$lines.Add("//   if (x4n::entity::is_derived_from(comp, x4n::GameClass::Container)) { ... }")
$lines.Add("//")
$lines.Add("// WARNING: Class IDs shift between game builds when new classes are inserted.")
$lines.Add("// DO NOT EDIT - regenerate with: .\scripts\generate_class_ids.ps1")
$lines.Add("// ==========================================================================")
$lines.Add("")

# ---- C++ enum class ----
$lines.Add("#ifdef __cplusplus")
$lines.Add("namespace x4n {")
$lines.Add("")
$lines.Add("/// Entity class ID enum. Values are auto-generated per game build.")
$lines.Add("/// Use with x4n::entity::get_class_id() and x4n::entity::is_derived_from().")
$lines.Add("enum class GameClass : uint32_t {")

$lastIdx = $entries.Count - 1
for ($i = 0; $i -le $lastIdx; $i++) {
    $e = $entries[$i]
    $padded = $e.PascalName.PadRight($maxPascalLen + 2)
    $comma = if ($i -lt $lastIdx) { "," } else { "" }
    $lines.Add("    ${padded}= $($e.Id)$comma")
}
$lines.Add("};")
$lines.Add("")
$lines.Add("/// Internal sentinel value -- not a real class. Returned by get_class() on failure.")
$lines.Add("/// Extension code should check for nullptr/null component instead of comparing to this.")
$lines.Add("inline constexpr uint32_t GAMECLASS_SENTINEL = $sentinel;")
$lines.Add("")
$lines.Add("} // namespace x4n")
$lines.Add("#endif")
$lines.Add("")

$content = $lines -join "`n"
Set-Content -Path $outFile -Value $content -Encoding UTF8 -NoNewline

Write-Host "  Generated $outFile ($($entries.Count) classes + sentinel)"
