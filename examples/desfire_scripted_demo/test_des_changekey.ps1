[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ComPort,

    [int]$Baud = 115200,

    [ValidateSet("legacy", "iso", "aes")]
    [string]$PiccAuthMode = "legacy",

    [ValidateRange(0, 14)]
    [int]$PiccAuthKeyNo = 0,

    [Parameter(Mandatory = $true)]
    [string]$PiccAuthKeyHex,

    [string]$DesAppAid = "A1A553",
    [string]$DesNewKeyHex = "D1 D2 D3 D4 D5 D6 D7 D8",

    [string]$ExeDir = "",
    [switch]$SkipDelete,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptRootPath = if ([string]::IsNullOrWhiteSpace($PSScriptRoot)) { (Get-Location).Path } else { $PSScriptRoot }
$repoRoot = (Resolve-Path (Join-Path $scriptRootPath "..\..")).Path
if ([string]::IsNullOrWhiteSpace($ExeDir)) {
    $ExeDir = Join-Path $scriptRootPath "..\..\build\examples\Debug"
}

function Get-NormalizedHex {
    param([Parameter(Mandatory = $true)][string]$Hex)
    return ($Hex -replace "[^0-9A-Fa-f]", "").ToUpperInvariant()
}

function Assert-HexLength {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Hex,
        [Parameter(Mandatory = $true)][int[]]$AllowedByteLengths
    )

    $normalized = Get-NormalizedHex -Hex $Hex
    if (($normalized.Length % 2) -ne 0) {
        throw "$Name has an odd number of hex digits."
    }

    $byteLength = $normalized.Length / 2
    if ($AllowedByteLengths -notcontains $byteLength) {
        $allowed = ($AllowedByteLengths | ForEach-Object { $_.ToString() }) -join ", "
        throw "$Name must be one of [$allowed] bytes, but got $byteLength bytes."
    }
}

function Resolve-Executable {
    param([Parameter(Mandatory = $true)][string]$ExecutableName)

    $path = Join-Path $ExeDir $ExecutableName
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Executable not found: $path"
    }
    return $path
}

function Assert-ExecutableFreshness {
    param(
        [Parameter(Mandatory = $true)][string]$ExecutableName,
        [Parameter(Mandatory = $true)][string[]]$SourceFiles
    )

    $exePath = Resolve-Executable -ExecutableName $ExecutableName
    $exeTimeUtc = (Get-Item -LiteralPath $exePath).LastWriteTimeUtc

    foreach ($sourceRelative in $SourceFiles) {
        $sourcePath = Join-Path $repoRoot $sourceRelative
        if (-not (Test-Path -LiteralPath $sourcePath)) {
            continue
        }

        $sourceTimeUtc = (Get-Item -LiteralPath $sourcePath).LastWriteTimeUtc
        if ($sourceTimeUtc -gt $exeTimeUtc) {
            throw ("Executable '{0}' is older than source '{1}'. Rebuild first: cmake --build build --config Debug --target {2}" -f
                $ExecutableName, $sourceRelative, [System.IO.Path]::GetFileNameWithoutExtension($ExecutableName))
        }
    }
}

function Build-RequiredTargets {
    param(
        [Parameter(Mandatory = $true)][string[]]$Targets,
        [switch]$CleanFirst
    )

    if ($SkipBuild) {
        return
    }

    Write-Host ""
    Write-Host "Building required targets..." -ForegroundColor Cyan
    $cleanText = if ($CleanFirst) { " with --clean-first" } else { "" }
    Write-Host ("  cmake --build build --config Debug --target {0}{1}" -f ($Targets -join " "), $cleanText) -ForegroundColor DarkGray

    $buildArgs = @("--build", "build", "--config", "Debug")
    if ($CleanFirst) {
        $buildArgs += "--clean-first"
    }
    $buildArgs += "--target"
    $buildArgs += $Targets

    Push-Location $repoRoot
    try {
        & cmake @buildArgs
        if ($LASTEXITCODE -ne 0) {
            throw ("Build failed with exit code {0}" -f $LASTEXITCODE)
        }
    }
    finally {
        Pop-Location
    }
}

$script:StepIndex = 0
function Invoke-Step {
    param(
        [Parameter(Mandatory = $true)][string]$Title,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $script:StepIndex++
    $exePath = Resolve-Executable -ExecutableName $Executable
    Write-Host ""
    Write-Host (("[{0:D2}] {1}" -f $script:StepIndex, $Title)) -ForegroundColor Cyan
    & $exePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw ("Step failed with exit code {0}: {1}" -f $LASTEXITCODE, $Title)
    }
}

$normalizedAid = Get-NormalizedHex -Hex $DesAppAid
if ($normalizedAid.Length -ne 6) {
    throw "DesAppAid must be exactly 3 bytes (6 hex chars)."
}

$piccAllowedKeySizes = switch ($PiccAuthMode) {
    "legacy" { @(8, 16) }
    "iso"    { @(8, 16, 24) }
    "aes"    { @(16) }
    default   { throw "Unexpected PiccAuthMode: $PiccAuthMode" }
}

Assert-HexLength -Name "PiccAuthKeyHex" -Hex $PiccAuthKeyHex -AllowedByteLengths $piccAllowedKeySizes
Assert-HexLength -Name "DesNewKeyHex" -Hex $DesNewKeyHex -AllowedByteLengths @(8)

Assert-ExecutableFreshness -ExecutableName "desfire_auth_changekey_example.exe" -SourceFiles @(
    "Src/Nfc/Desfire/Commands/AuthenticateCommand.cpp",
    "Src/Nfc/Desfire/Commands/ChangeKeyCommand.cpp"
)
Assert-ExecutableFreshness -ExecutableName "desfire_get_key_info_example.exe" -SourceFiles @(
    "Src/Nfc/Desfire/Commands/AuthenticateCommand.cpp",
    "Src/Nfc/Desfire/Commands/GetKeySettingsCommand.cpp",
    "Src/Nfc/Desfire/Commands/GetKeyVersionCommand.cpp"
)

Build-RequiredTargets -Targets @(
    "desfire_auth_changekey_example",
    "desfire_get_key_info_example",
    "desfire_create_application_example",
    "desfire_delete_application_example"
) -CleanFirst

$zeroDesKey = "00 00 00 00 00 00 00 00"

Write-Host "DES ChangeKey Focused Test" -ForegroundColor Green
Write-Host "  COM port: $ComPort"
Write-Host "  Baud: $Baud"
Write-Host "  App AID: $normalizedAid"
Write-Host "  Exe dir: $ExeDir"

if (-not $SkipDelete) {
    Invoke-Step -Title "Delete DES app if it exists" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedAid,
        "--allow-missing"
    )
}

Invoke-Step -Title "Create DES app with default zero key" -Executable "desfire_create_application_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--picc-auth-mode", $PiccAuthMode,
    "--picc-auth-key-no", "$PiccAuthKeyNo",
    "--picc-auth-key-hex", $PiccAuthKeyHex,
    "--app-aid", $normalizedAid,
    "--app-key-settings", "0x0F",
    "--app-key-count", "1",
    "--app-key-type", "des",
    "--app-auth-mode", "legacy",
    "--app-auth-key-no", "0",
    "--app-auth-key-hex", $zeroDesKey,
    "--allow-existing"
)

Invoke-Step -Title "Rotate DES app key 0 from zero key to target key" -Executable "desfire_auth_changekey_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAid,
    "--auth-mode", "legacy",
    "--current-key-type", "des",
    "--auth-key-no", "0",
    "--auth-key-hex", $zeroDesKey,
    "--change-key-no", "0",
    "--new-key-type", "des",
    "--new-key-hex", $DesNewKeyHex,
    "--new-key-version", "0",
    "--confirm-change"
)

Invoke-Step -Title "Verify auth and key info with rotated DES key" -Executable "desfire_get_key_info_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAid,
    "--authenticate",
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesNewKeyHex,
    "--query-key-no", "0"
)

Write-Host ""
Write-Host "DES ChangeKey focused test completed successfully." -ForegroundColor Green
