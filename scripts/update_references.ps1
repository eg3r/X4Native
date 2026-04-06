<#
.SYNOPSIS
    Master script: update all X4 reference data after a game patch.

.DESCRIPTION
    Runs the full reference extraction pipeline in order:
      1. extract_game_files.ps1  — Lua/XML from cat/dat archives
      2. extract_exports.ps1     — PE export table from X4.exe
      3. extract_ffi.ps1         — FFI cdef parsing and cross-reference
      4. generate_headers.ps1    — Auto-generate C headers from FFI data
      5. generate_version_db.ps1 — Version metadata (func_history, type_changes)

    All parameters are auto-detected but can be overridden.
    After running, use 'git diff reference/' to review changes.

.PARAMETER GameDir
    Path to X4: Foundations install. Auto-detected from Steam registry if omitted.

.PARAMETER ToolDir
    Path to X Tools install (contains XRCatTool.exe). Auto-detected if omitted.

.PARAMETER DumpbinPath
    Full path to dumpbin.exe. Auto-detected via vswhere if omitted.

.PARAMETER SkipGameFiles
    Skip the game file extraction step (useful if you only want to refresh exports/FFI).

.PARAMETER SkipExports
    Skip the PE export extraction step.

.PARAMETER SkipFFI
    Skip the FFI extraction step.

.PARAMETER SkipHeaders
    Skip the header generation step.

.PARAMETER VersionSuffix
    Optional suffix appended to the numeric version from version.dat, joined with a dash.
    Overrides auto-detection. Use when the byte-scan heuristic fails or you need a
    human-readable label instead of the build number.
    Example: -VersionSuffix beta2  =>  stored version becomes "900-beta2"

.PARAMETER SkipVersionDb
    Skip the version_db metadata generation step.

.EXAMPLE
    .\scripts\update_references.ps1
    .\scripts\update_references.ps1 -SkipGameFiles
    .\scripts\update_references.ps1 -GameDir "D:\Games\X4 Foundations"
    .\scripts\update_references.ps1 -VersionSuffix beta2
#>
param(
    [string]$GameDir,
    [string]$ToolDir,
    [string]$DumpbinPath,
    [string]$VersionSuffix,
    [switch]$SkipGameFiles,
    [switch]$SkipExports,
    [switch]$SkipFFI,
    [switch]$SkipHeaders,
    [switch]$SkipVersionDb,
    [switch]$SkipGameData
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# Scans the .text section of X4.exe for the consecutive pair:
#   mov [rip+disp32], <version>    ; C7 05 [4] [version LE]
#   mov [rip+disp32], <build>      ; C7 05 [4] [build LE]
# Returns the build number int32, or $null if not found / implausible.
function Get-X4BuildNumber {
    param([string]$ExePath, [int]$Version)

    $stream = [System.IO.File]::OpenRead($ExePath)
    $reader = [System.IO.BinaryReader]::new($stream)
    try {
        # Locate PE header
        $stream.Seek(0x3C, 'Begin') | Out-Null
        $peOffset = $reader.ReadInt32()

        # Number of sections
        $stream.Seek($peOffset + 6, 'Begin') | Out-Null
        $numSections = $reader.ReadUInt16()

        # Optional header size → sections start after it
        $stream.Seek($peOffset + 20, 'Begin') | Out-Null
        $optHeaderSize = $reader.ReadUInt16()
        $sectionsBase  = $peOffset + 24 + $optHeaderSize

        # Find .text section raw offset + size
        $textOffset = 0; $textSize = 0
        for ($s = 0; $s -lt $numSections; $s++) {
            $stream.Seek($sectionsBase + $s * 40, 'Begin') | Out-Null
            $name    = [System.Text.Encoding]::ASCII.GetString($reader.ReadBytes(8)).TrimEnd([char]0)
            $null    = $reader.ReadUInt32()   # virtual size
            $null    = $reader.ReadUInt32()   # virtual address
            $rawSize = $reader.ReadUInt32()
            $rawOff  = $reader.ReadUInt32()
            if ($name -eq '.text') { $textOffset = $rawOff; $textSize = $rawSize; break }
        }
        if ($textSize -eq 0) { return $null }

        $stream.Seek($textOffset, 'Begin') | Out-Null
        $text = $reader.ReadBytes($textSize)

        # Build LE byte array for the version anchor
        $vLE = [BitConverter]::GetBytes([int32]$Version)

        for ($i = 0; $i -le $text.Length - 20; $i++) {
            if ($text[$i]    -eq 0xC7 -and $text[$i+1]  -eq 0x05 -and
                $text[$i+6]  -eq $vLE[0] -and $text[$i+7]  -eq $vLE[1] -and
                $text[$i+8]  -eq $vLE[2] -and $text[$i+9]  -eq $vLE[3] -and
                $text[$i+10] -eq 0xC7   -and $text[$i+11] -eq 0x05) {
                $build = [BitConverter]::ToInt32($text, $i + 16)
                if ($build -gt 100000 -and $build -lt 9999999) { return $build }
            }
        }
        return $null
    } finally {
        $reader.Dispose()
        $stream.Dispose()
    }
}

# ---------------------------------------------------------------------------
# Auto-detect game directory (shared across steps)
# ---------------------------------------------------------------------------
if (-not $GameDir) {
    $steamPath = (Get-ItemProperty -Path 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam' -Name InstallPath -ErrorAction SilentlyContinue).InstallPath
    if ($steamPath) {
        $candidate = Join-Path $steamPath 'steamapps\common\X4 Foundations'
        if (Test-Path "$candidate\X4.exe") { $GameDir = $candidate }
    }
    if (-not $GameDir) {
        Write-Error "Cannot auto-detect X4 install. Pass -GameDir explicitly."
        exit 1
    }
}

# Read game version from version.dat, then determine the full version label.
# Priority: 1) explicit -VersionSuffix  2) build number from X4.exe  3) raw version only
$versionFile = Join-Path $GameDir 'version.dat'
$rawVersion  = if (Test-Path $versionFile) { (Get-Content $versionFile -Raw).Trim() } else { 'unknown' }

$detectedSuffix = $null
if (-not $VersionSuffix -and $rawVersion -match '^\d+$') {
    Write-Host "  Scanning X4.exe for build number..." -NoNewline
    $exeBuildNumber = Get-X4BuildNumber -ExePath (Join-Path $GameDir 'X4.exe') -Version ([int]$rawVersion)
    if ($exeBuildNumber) {
        $detectedSuffix = "$exeBuildNumber"
        Write-Host " $exeBuildNumber" -ForegroundColor Green
    } else {
        Write-Host " not found (use -VersionSuffix to set manually)" -ForegroundColor Yellow
    }
}

$activeSuffix = if ($VersionSuffix) { $VersionSuffix } else { $detectedSuffix }
$gameVersion  = if ($activeSuffix) { "$rawVersion-$activeSuffix" } else { $rawVersion }

$versionDisplay = if ($rawVersion -match '^\d+$' -and $rawVersion.Length -ge 3) {
    $pretty = "v$($rawVersion.Substring(0, $rawVersion.Length - 2)).$($rawVersion.Substring($rawVersion.Length - 2))"
    if ($activeSuffix) { "$pretty-$activeSuffix" } else { "$pretty (build $rawVersion)" }
} else {
    $gameVersion
}

Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  X4Native Reference Update Pipeline" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  Game dir : $GameDir"
Write-Host "  Version  : $versionDisplay"
Write-Host "  Repo     : $repoRoot"
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""

$steps = @()
if (-not $SkipGameFiles) { $steps += 'game_files' }
if (-not $SkipExports)   { $steps += 'exports' }
if (-not $SkipFFI)       { $steps += 'ffi' }
if (-not $SkipHeaders)   { $steps += 'headers' }
if (-not $SkipHeaders)   { $steps += 'class_ids' }
if (-not $SkipHeaders)   { $steps += 'event_type_ids' }
if (-not $SkipVersionDb) { $steps += 'version_db' }
if (-not $SkipGameData)  { $steps += 'game_data' }

if ($steps.Count -eq 0) {
    Write-Host "All steps skipped. Nothing to do." -ForegroundColor Yellow
    exit 0
}

$stepNum = 0
$totalSteps = $steps.Count

# ---------------------------------------------------------------------------
# Step 1: Extract game files
# ---------------------------------------------------------------------------
if ($steps -contains 'game_files') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Extracting game files..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    $gameFileArgs = @{}
    if ($GameDir)      { $gameFileArgs['GameDir']      = $GameDir }
    if ($ToolDir)      { $gameFileArgs['ToolDir']      = $ToolDir }
    if ($gameVersion)  { $gameFileArgs['GameVersion']  = $gameVersion }

    & "$PSScriptRoot\extract_game_files.ps1" @gameFileArgs
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "extract_game_files.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 2: Extract PE exports
# ---------------------------------------------------------------------------
if ($steps -contains 'exports') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Extracting PE exports..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    $exportArgs = @{}
    if ($GameDir)      { $exportArgs['GameDir'] = $GameDir }
    if ($DumpbinPath)  { $exportArgs['DumpbinPath'] = $DumpbinPath }

    & "$PSScriptRoot\extract_exports.ps1" @exportArgs
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "extract_exports.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 3: Extract FFI declarations and cross-reference
# ---------------------------------------------------------------------------
if ($steps -contains 'ffi') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Extracting FFI declarations..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    & "$PSScriptRoot\extract_ffi.ps1"
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "extract_ffi.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 4: Generate C headers
# ---------------------------------------------------------------------------
if ($steps -contains 'headers') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Generating C headers..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    $headerArgs = @{}
    if ($gameVersion) { $headerArgs['GameVersion'] = $gameVersion }

    & "$PSScriptRoot\generate_headers.ps1" @headerArgs
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "generate_headers.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 5: Generate class ID defines
# ---------------------------------------------------------------------------
if ($steps -contains 'class_ids') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Generating class ID defines..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    $classArgs = @{}
    if ($gameVersion) { $classArgs['GameVersion'] = $gameVersion }

    & "$PSScriptRoot\generate_class_ids.ps1" @classArgs
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "generate_class_ids.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 6: Generate event type ID defines
# ---------------------------------------------------------------------------
if ($steps -contains 'event_type_ids') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Generating event type ID defines..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    $eventArgs = @{}
    if ($gameVersion) { $eventArgs['GameVersion'] = $gameVersion }

    & "$PSScriptRoot\generate_event_type_ids.ps1" @eventArgs
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "generate_event_type_ids.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 7: Generate version_db metadata
# ---------------------------------------------------------------------------
if ($steps -contains 'version_db') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Generating version_db metadata..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    $vdbArgs = @{}
    if ($gameVersion) { $vdbArgs['GameVersion'] = $gameVersion }

    & "$PSScriptRoot\generate_version_db.ps1" @vdbArgs
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "generate_version_db.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 8: Generate game data lookup tables
# ---------------------------------------------------------------------------
if ($steps -contains 'game_data') {
    $stepNum++
    Write-Host "[$stepNum/$totalSteps] Generating game data tables..." -ForegroundColor Cyan
    Write-Host "------------------------------------------------------------"

    & "$PSScriptRoot\generate_game_data.ps1"
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Error "generate_game_data.ps1 failed"
        exit 1
    }
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  Pipeline Complete" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""

# Show reference file sizes
$refDir = Join-Path $repoRoot 'reference'
$refFiles = @(
    'x4_exports.txt',
    'x4_ffi_raw.txt',
    'x4_struct_names.txt',
    'x4_ffi_summary.txt'
)
foreach ($f in $refFiles) {
    $path = Join-Path $refDir $f
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        $sizeKB = [math]::Round($size / 1024, 1)
        Write-Host "  $($f.PadRight(25)) ${sizeKB} KB"
    }
}

$gameFileCount = if (Test-Path (Join-Path $refDir 'game')) {
    (Get-ChildItem (Join-Path $refDir 'game') -Recurse -File).Count
} else { 0 }
Write-Host "  game/ files              $gameFileCount"

# Show generated header sizes
$sdkDir = Join-Path $repoRoot 'sdk'
$headerFiles = @('x4_game_types.h', 'x4_game_func_list.inc', 'x4_game_func_table.h', 'x4_game_class_ids.inc', 'x4_md_events.h')
foreach ($f in $headerFiles) {
    $path = Join-Path $sdkDir $f
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        $sizeKB = [math]::Round($size / 1024, 1)
        Write-Host "  sdk/$($f.PadRight(23)) ${sizeKB} KB"
    }
}

# Show version_db sizes
$vdbDir = Join-Path $repoRoot 'native\version_db'
foreach ($f in @('func_history.json', 'type_changes.json')) {
    $path = Join-Path $vdbDir $f
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        $sizeKB = [math]::Round($size / 1024, 1)
        Write-Host "  version_db/$($f.PadRight(18)) ${sizeKB} KB"
    }
}

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  git diff reference/ sdk/                      # Review changes"
Write-Host "  git add reference/ sdk/ ; git commit -m 'reference: update to $versionDisplay'"
Write-Host ""
