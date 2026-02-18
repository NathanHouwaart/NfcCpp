[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ComPort,

    [int]$Baud = 115200,

    [string]$ExeDir = "",

    [ValidateSet("legacy", "iso", "aes")]
    [string]$PiccAuthMode = "iso",

    [ValidateRange(0, 14)]
    [int]$PiccAuthKeyNo = 0,

    [Parameter(Mandatory = $true)]
    [string]$PiccAuthKeyHex,

    [string]$AesAppAid = "A1A551",
    [string]$AesAppKeyHex = "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00",

    [string]$ThreeDesAppAid = "A1A552",
    [string]$ThreeDesAppKeyHex = "10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27",

    [string]$DesAppAid = "A1A553",
    [string]$DesAppKeyHex = "D1 D2 D3 D4 D5 D6 D7 D8",

    [string]$TwoK3DesAppAid = "A1A554",
    [string]$TwoK3DesAppKeyHex = "21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30",

    [switch]$SkipInitialCleanup,
    [switch]$CleanupAtEnd,
    [switch]$RunSetConfiguration,
    [switch]$RunFormatPicc,
    [switch]$ConfirmDangerousPiccOperations,
    [switch]$DryRun,
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

function Assert-NotAllZeroHex {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Hex
    )

    $normalized = Get-NormalizedHex -Hex $Hex
    if ($normalized -match "^[0]+$") {
        throw "$Name must not be all 00 for this demo."
    }
}

function Assert-Aid {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Aid
    )

    $normalized = Get-NormalizedHex -Hex $Aid
    if ($normalized.Length -ne 6) {
        throw "$Name must be exactly 3 bytes (6 hex chars)."
    }
    return $normalized
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

    if ($DryRun -or $SkipBuild) {
        return
    }

    Write-Host ""
    Write-Host "Building required targets..." -ForegroundColor Cyan
    $cleanText = if ($CleanFirst) { " with --clean-first" } else { "" }
    Write-Host ("  cmake --build build --config Debug --target {0}{1}" -f ($Targets -join " "), $cleanText) -ForegroundColor DarkGray

    $buildArgs = @("--build", "build", "--config", "Debug")
    # if ($CleanFirst) {
    #     $buildArgs += "--clean-first"
    # }
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
function Invoke-DemoCommand {
    param(
        [Parameter(Mandatory = $true)][string]$Title,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $script:StepIndex++
    $exePath = Resolve-Executable -ExecutableName $Executable

    Write-Host ""
    Write-Host (("[{0:D2}] {1}" -f $script:StepIndex, $Title)) -ForegroundColor Cyan

    $printableArgs = $Arguments | ForEach-Object {
        if ($_ -match "\s") { '"{0}"' -f $_ } else { $_ }
    }
    Write-Host ("    " + $exePath + " " + ($printableArgs -join " ")) -ForegroundColor DarkGray

    if ($DryRun) {
        return
    }

    & $exePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw ("Step failed with exit code {0}: {1}" -f $LASTEXITCODE, $Title)
    }
}

$normalizedAesAid = Assert-Aid -Name "AesAppAid" -Aid $AesAppAid
$normalizedThreeDesAid = Assert-Aid -Name "ThreeDesAppAid" -Aid $ThreeDesAppAid
$normalizedDesAid = Assert-Aid -Name "DesAppAid" -Aid $DesAppAid
$normalizedTwoK3DesAid = Assert-Aid -Name "TwoK3DesAppAid" -Aid $TwoK3DesAppAid

$demoAids = @(
    $normalizedAesAid,
    $normalizedThreeDesAid,
    $normalizedDesAid,
    $normalizedTwoK3DesAid
)

if ($demoAids | Where-Object { $_ -eq "000000" }) {
    throw "Demo application AIDs must not be 000000 (PICC)."
}

if (($demoAids | Select-Object -Unique).Count -ne $demoAids.Count) {
    throw "All demo application AIDs must be unique."
}

$piccAllowedKeySizes = switch ($PiccAuthMode) {
    "legacy" { @(8, 16) }
    "iso"    { @(8, 16, 24) }
    "aes"    { @(16) }
    default   { throw "Unexpected PiccAuthMode: $PiccAuthMode" }
}

Assert-HexLength -Name "PiccAuthKeyHex" -Hex $PiccAuthKeyHex -AllowedByteLengths $piccAllowedKeySizes
Assert-HexLength -Name "AesAppKeyHex" -Hex $AesAppKeyHex -AllowedByteLengths @(16)
Assert-HexLength -Name "ThreeDesAppKeyHex" -Hex $ThreeDesAppKeyHex -AllowedByteLengths @(24)
Assert-HexLength -Name "DesAppKeyHex" -Hex $DesAppKeyHex -AllowedByteLengths @(8)
Assert-HexLength -Name "TwoK3DesAppKeyHex" -Hex $TwoK3DesAppKeyHex -AllowedByteLengths @(16)

Assert-NotAllZeroHex -Name "AesAppKeyHex" -Hex $AesAppKeyHex
Assert-NotAllZeroHex -Name "ThreeDesAppKeyHex" -Hex $ThreeDesAppKeyHex
Assert-NotAllZeroHex -Name "DesAppKeyHex" -Hex $DesAppKeyHex
Assert-NotAllZeroHex -Name "TwoK3DesAppKeyHex" -Hex $TwoK3DesAppKeyHex

Assert-ExecutableFreshness -ExecutableName "desfire_auth_changekey_example.exe" -SourceFiles @(
    "Src/Nfc/Desfire/Commands/AuthenticateCommand.cpp",
    "Src/Nfc/Desfire/Commands/ChangeKeyCommand.cpp"
)
Assert-ExecutableFreshness -ExecutableName "desfire_get_key_info_example.exe" -SourceFiles @(
    "Src/Nfc/Desfire/Commands/AuthenticateCommand.cpp",
    "Src/Nfc/Desfire/Commands/GetKeySettingsCommand.cpp",
    "Src/Nfc/Desfire/Commands/GetKeyVersionCommand.cpp"
)
Assert-ExecutableFreshness -ExecutableName "desfire_change_key_settings_example.exe" -SourceFiles @(
    "Src/Nfc/Desfire/Commands/AuthenticateCommand.cpp",
    "Src/Nfc/Desfire/Commands/ChangeKeySettingsCommand.cpp"
)
Assert-ExecutableFreshness -ExecutableName "desfire_read_write_data_example.exe" -SourceFiles @(
    "Src/Nfc/Desfire/Commands/ReadDataCommand.cpp",
    "Src/Nfc/Desfire/Commands/WriteDataCommand.cpp",
    "Src/Nfc/Desfire/DesfireCard.cpp"
)

Build-RequiredTargets -Targets @(
    "desfire_auth_changekey_example",
    "desfire_get_key_info_example",
    "desfire_change_key_settings_example",
    "desfire_read_write_data_example"
) -CleanFirst

if (($RunSetConfiguration -or $RunFormatPicc) -and -not $ConfirmDangerousPiccOperations) {
    throw "Use -ConfirmDangerousPiccOperations when enabling -RunSetConfiguration and/or -RunFormatPicc."
}

$zeroAesKey = "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
$zero3k3desKey = "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
$zeroDesKey = "00 00 00 00 00 00 00 00"
$zero2k3desKey = "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"

$piccAuthArgs = @(
    "--authenticate",
    "--auth-mode", $PiccAuthMode,
    "--auth-key-no", "$PiccAuthKeyNo",
    "--auth-key-hex", $PiccAuthKeyHex
)

$aesAppAuthArgs = @(
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex
)
$aesAuthOnlyArgs = @(
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex
)

$threeDesAppAuthArgs = @(
    "--aid", $normalizedThreeDesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex
)
$threeDesAuthOnlyArgs = @(
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex
)

$desAppAuthArgs = @(
    "--aid", $normalizedDesAid,
    "--authenticate",
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex
)
$desAuthOnlyArgs = @(
    "--authenticate",
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex
)

$twoK3DesAppAuthArgs = @(
    "--aid", $normalizedTwoK3DesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex
)
$twoK3DesAuthOnlyArgs = @(
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex
)

Write-Host "DESFire Scripted Demo" -ForegroundColor Green
Write-Host "  COM port: $ComPort"
Write-Host "  Baud: $Baud"
Write-Host "  AES app AID: $normalizedAesAid"
Write-Host "  3K3DES app AID: $normalizedThreeDesAid"
Write-Host "  DES app AID: $normalizedDesAid"
Write-Host "  2K3DES app AID: $normalizedTwoK3DesAid"
Write-Host "  Executable dir: $ExeDir"
if ($DryRun) {
    Write-Host "  Mode: DRY RUN (commands are printed, not executed)" -ForegroundColor Yellow
}

# Stage 1: PICC-level discovery and health checks.
Invoke-DemoCommand -Title "GetVersion" -Executable "desfire_get_version_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud"
)

Invoke-DemoCommand -Title "GetApplicationIDs (PICC auth)" -Executable "desfire_get_application_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $piccAuthArgs)

Invoke-DemoCommand -Title "GetCardUID (PICC auth)" -Executable "desfire_get_card_uid_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--auth-mode", $PiccAuthMode,
    "--auth-key-no", "$PiccAuthKeyNo",
    "--auth-key-hex", $PiccAuthKeyHex
)

Invoke-DemoCommand -Title "Get PICC key settings + key version" -Executable "desfire_get_key_info_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", "000000",
    "--authenticate",
    "--auth-mode", $PiccAuthMode,
    "--auth-key-no", "$PiccAuthKeyNo",
    "--auth-key-hex", $PiccAuthKeyHex,
    "--query-key-no", "0"
)

Invoke-DemoCommand -Title "FreeMemory (before demo)" -Executable "desfire_free_memory_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud",
    "--aid", "000000"
) + $piccAuthArgs)

# Stage 2: Optional cleanup and app creation.
if (-not $SkipInitialCleanup) {
    Invoke-DemoCommand -Title "Delete AES app if it exists" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedAesAid,
        "--allow-missing"
    )

    Invoke-DemoCommand -Title "Delete 3K3DES app if it exists" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedThreeDesAid,
        "--allow-missing"
    )

    Invoke-DemoCommand -Title "Delete DES app if it exists" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedDesAid,
        "--allow-missing"
    )

    Invoke-DemoCommand -Title "Delete 2K3DES app if it exists" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedTwoK3DesAid,
        "--allow-missing"
    )
}

Invoke-DemoCommand -Title "Create AES application" -Executable "desfire_create_application_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--picc-auth-mode", $PiccAuthMode,
    "--picc-auth-key-no", "$PiccAuthKeyNo",
    "--picc-auth-key-hex", $PiccAuthKeyHex,
    "--app-aid", $normalizedAesAid,
    "--app-key-settings", "0x0F",
    "--app-key-count", "1",
    "--app-key-type", "aes",
    "--app-auth-mode", "aes",
    "--app-auth-key-no", "0",
    "--app-auth-key-hex", $zeroAesKey,
    "--allow-existing"
)

Invoke-DemoCommand -Title "Create 3K3DES application" -Executable "desfire_create_application_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--picc-auth-mode", $PiccAuthMode,
    "--picc-auth-key-no", "$PiccAuthKeyNo",
    "--picc-auth-key-hex", $PiccAuthKeyHex,
    "--app-aid", $normalizedThreeDesAid,
    "--app-key-settings", "0x0F",
    "--app-key-count", "1",
    "--app-key-type", "3k3des",
    "--app-auth-mode", "iso",
    "--app-auth-key-no", "0",
    "--app-auth-key-hex", $zero3k3desKey,
    "--allow-existing"
)

Invoke-DemoCommand -Title "Create DES application" -Executable "desfire_create_application_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--picc-auth-mode", $PiccAuthMode,
    "--picc-auth-key-no", "$PiccAuthKeyNo",
    "--picc-auth-key-hex", $PiccAuthKeyHex,
    "--app-aid", $normalizedDesAid,
    "--app-key-settings", "0x0F",
    "--app-key-count", "1",
    "--app-key-type", "des",
    "--app-auth-mode", "legacy",
    "--app-auth-key-no", "0",
    "--app-auth-key-hex", $zeroDesKey,
    "--allow-existing"
)

Invoke-DemoCommand -Title "Create 2K3DES application" -Executable "desfire_create_application_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--picc-auth-mode", $PiccAuthMode,
    "--picc-auth-key-no", "$PiccAuthKeyNo",
    "--picc-auth-key-hex", $PiccAuthKeyHex,
    "--app-aid", $normalizedTwoK3DesAid,
    "--app-key-settings", "0x0F",
    "--app-key-count", "1",
    "--app-key-type", "2k3des",
    "--app-auth-mode", "iso",
    "--app-auth-key-no", "0",
    "--app-auth-key-hex", $zero2k3desKey,
    "--allow-existing"
)

Invoke-DemoCommand -Title "Rotate AES app key 0 from zero key to demo AES key" -Executable "desfire_auth_changekey_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--auth-mode", "aes",
    "--current-key-type", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $zeroAesKey,
    "--change-key-no", "0",
    "--new-key-type", "aes",
    "--new-key-hex", $AesAppKeyHex,
    "--new-key-version", "0",
    "--confirm-change"
)

Invoke-DemoCommand -Title "Rotate 3K3DES app key 0 from zero key to demo 3K3DES key" -Executable "desfire_auth_changekey_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--auth-mode", "iso",
    "--current-key-type", "3k3des",
    "--auth-key-no", "0",
    "--auth-key-hex", $zero3k3desKey,
    "--change-key-no", "0",
    "--new-key-type", "3k3des",
    "--new-key-hex", $ThreeDesAppKeyHex,
    "--new-key-version", "0",
    "--confirm-change"
)

Invoke-DemoCommand -Title "Rotate DES app key 0 from zero key to demo DES key" -Executable "desfire_auth_changekey_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--auth-mode", "legacy",
    "--current-key-type", "des",
    "--auth-key-no", "0",
    "--auth-key-hex", $zeroDesKey,
    "--change-key-no", "0",
    "--new-key-type", "des",
    "--new-key-hex", $DesAppKeyHex,
    "--new-key-version", "0",
    "--confirm-change"
)

Invoke-DemoCommand -Title "Rotate 2K3DES app key 0 from zero key to demo 2K3DES key" -Executable "desfire_auth_changekey_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--auth-mode", "iso",
    "--current-key-type", "2k3des",
    "--auth-key-no", "0",
    "--auth-key-hex", $zero2k3desKey,
    "--change-key-no", "0",
    "--new-key-type", "2k3des",
    "--new-key-hex", $TwoK3DesAppKeyHex,
    "--new-key-version", "0",
    "--confirm-change"
)

Invoke-DemoCommand -Title "List apps after creation" -Executable "desfire_get_application_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $piccAuthArgs)

Invoke-DemoCommand -Title "Check AES app key settings + key version" -Executable "desfire_get_key_info_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--query-key-no", "0"
)

Invoke-DemoCommand -Title "Check 3K3DES app key settings + key version" -Executable "desfire_get_key_info_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--query-key-no", "0"
)

Invoke-DemoCommand -Title "Check DES app key settings + key version" -Executable "desfire_get_key_info_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--authenticate",
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex,
    "--query-key-no", "0"
)

Invoke-DemoCommand -Title "Check 2K3DES app key settings + key version" -Executable "desfire_get_key_info_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex,
    "--query-key-no", "0"
)

Invoke-DemoCommand -Title "Apply ChangeKeySettings on AES app (0x0F)" -Executable "desfire_change_key_settings_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--key-settings", "0x0F",
    "--confirm-change"
)

Invoke-DemoCommand -Title "Apply ChangeKeySettings on 3K3DES app (0x0F)" -Executable "desfire_change_key_settings_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--key-settings", "0x0F",
    "--confirm-change"
)

Invoke-DemoCommand -Title "Apply ChangeKeySettings on DES app (0x0F)" -Executable "desfire_change_key_settings_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex,
    "--key-settings", "0x0F",
    "--confirm-change"
)

Invoke-DemoCommand -Title "Apply ChangeKeySettings on 2K3DES app (0x0F)" -Executable "desfire_change_key_settings_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex,
    "--key-settings", "0x0F",
    "--confirm-change"
)

# Stage 3: AES app file creation and usage.
Invoke-DemoCommand -Title "AES app: CreateStdDataFile (file 1)" -Executable "desfire_create_std_data_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "1",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--file-size", "64",
    "--allow-existing"
)

Invoke-DemoCommand -Title "AES app: CreateBackupDataFile (file 2)" -Executable "desfire_create_backup_data_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "2",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--file-size", "64",
    "--allow-existing"
)

Invoke-DemoCommand -Title "AES app: CreateValueFile (file 3)" -Executable "desfire_create_value_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "3",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--lower-limit", "-1000",
    "--upper-limit", "1000",
    "--limited-credit-value", "100",
    "--limited-credit-enabled",
    "--allow-existing"
)

Invoke-DemoCommand -Title "AES app: CreateLinearRecordFile (file 4)" -Executable "desfire_create_linear_record_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "4",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--record-size", "16",
    "--max-records", "20",
    "--allow-existing"
)

Invoke-DemoCommand -Title "AES app: CreateCyclicRecordFile (file 5)" -Executable "desfire_create_cyclic_record_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "5",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--record-size", "16",
    "--max-records", "20",
    "--allow-existing"
)

Invoke-DemoCommand -Title "AES app: GetFileIDs" -Executable "desfire_get_file_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $aesAppAuthArgs)

Invoke-DemoCommand -Title "AES app: GetFileSettings file 1" -Executable "desfire_get_file_settings_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--file-no", "1"
) + $aesAuthOnlyArgs)

Invoke-DemoCommand -Title "AES app: GetFileSettings file 3" -Executable "desfire_get_file_settings_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--file-no", "3"
) + $aesAuthOnlyArgs)

Invoke-DemoCommand -Title "AES app: Write+Read standard file (file 1)" -Executable "desfire_read_write_data_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "1",
    "--write-offset", "0",
    "--write-hex", "48 65 6C 6C 6F 20 41 45 53 20 53 54 44"
)

Invoke-DemoCommand -Title "AES app: Write backup file (file 2)" -Executable "desfire_read_write_data_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "2",
    "--write-offset", "0",
    "--write-hex", "42 41 43 4B 55 50 2D 41 45 53"
)

Invoke-DemoCommand -Title "AES app: Commit transaction (for backup/value/record writes)" -Executable "desfire_commit_transaction_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--allow-no-changes"
)

Invoke-DemoCommand -Title "AES app: Read backup file (file 2)" -Executable "desfire_read_write_data_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "2",
    "--read-offset", "0",
    "--read-length", "10"
)

Invoke-DemoCommand -Title "AES app: GetValue (file 3)" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "3",
    "--op", "get"
)

Invoke-DemoCommand -Title "AES app: Credit 25 on value file" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "3",
    "--op", "credit",
    "--value", "25",
    "--commit",
    "--show-before",
    "--show-after"
)

Invoke-DemoCommand -Title "AES app: Debit 10 on value file" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "3",
    "--op", "debit",
    "--value", "10",
    "--commit",
    "--show-before",
    "--show-after"
)

Invoke-DemoCommand -Title "AES app: LimitedCredit 5 on value file" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "3",
    "--op", "limited-credit",
    "--value", "5",
    "--commit",
    "--show-before",
    "--show-after"
)

Invoke-DemoCommand -Title "AES app: Write and read one record (file 5)" -Executable "desfire_record_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "5",
    "--write-offset", "0",
    "--write-hex", "01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10",
    "--commit",
    "--read-offset", "0",
    "--read-count", "1"
)

Invoke-DemoCommand -Title "AES app: ClearRecordFile (file 5) and commit" -Executable "desfire_clear_record_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "5",
    "--commit",
    "--allow-no-changes"
)

Invoke-DemoCommand -Title "AES app: ChangeFileSettings (file 1, keep key0 rights, encrypted mode)" -Executable "desfire_change_file_settings_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--file-no", "1",
    "--new-comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--command-comm-mode", "auto",
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--show-before",
    "--show-after",
    "--confirm-change"
)

Invoke-DemoCommand -Title "AES app: DeleteFile (file 4)" -Executable "desfire_delete_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedAesAid,
    "--authenticate",
    "--auth-mode", "aes",
    "--auth-key-no", "0",
    "--auth-key-hex", $AesAppKeyHex,
    "--file-no", "4",
    "--allow-missing"
)

Invoke-DemoCommand -Title "AES app: GetFileIDs after delete" -Executable "desfire_get_file_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $aesAppAuthArgs)

# Stage 4: 3K3DES app file creation and usage.
Invoke-DemoCommand -Title "3K3DES app: CreateStdDataFile (file 1)" -Executable "desfire_create_std_data_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--file-no", "1",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--file-size", "48",
    "--allow-existing"
)

Invoke-DemoCommand -Title "3K3DES app: CreateValueFile (file 3)" -Executable "desfire_create_value_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--file-no", "3",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--lower-limit", "0",
    "--upper-limit", "5000",
    "--limited-credit-value", "100",
    "--limited-credit-enabled",
    "--allow-existing"
)

Invoke-DemoCommand -Title "3K3DES app: CreateCyclicRecordFile (file 5)" -Executable "desfire_create_cyclic_record_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--file-no", "5",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--record-size", "16",
    "--max-records", "10",
    "--allow-existing"
)

Invoke-DemoCommand -Title "3K3DES app: GetFileIDs" -Executable "desfire_get_file_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $threeDesAppAuthArgs)

Invoke-DemoCommand -Title "3K3DES app: Write+Read standard file (file 1)" -Executable "desfire_read_write_data_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--file-no", "1",
    "--write-offset", "0",
    "--write-hex", "54 68 72 65 65 44 45 53 20 44 61 74 61"
)

Invoke-DemoCommand -Title "3K3DES app: Credit 100 on value file" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--file-no", "3",
    "--op", "credit",
    "--value", "100",
    "--commit",
    "--show-before",
    "--show-after"
)

Invoke-DemoCommand -Title "3K3DES app: Debit 20 on value file" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--file-no", "3",
    "--op", "debit",
    "--value", "20",
    "--commit",
    "--show-before",
    "--show-after"
)

Invoke-DemoCommand -Title "3K3DES app: Write and read one record (file 5)" -Executable "desfire_record_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $ThreeDesAppKeyHex,
    "--file-no", "5",
    "--write-offset", "0",
    "--write-hex", "AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99",
    "--commit",
    "--read-offset", "0",
    "--read-count", "1"
)

Invoke-DemoCommand -Title "3K3DES app: GetFileSettings (file 5)" -Executable "desfire_get_file_settings_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedThreeDesAid,
    "--file-no", "5"
) + $threeDesAuthOnlyArgs)

# Stage 5: DES app file creation and usage.
Invoke-DemoCommand -Title "DES app: CreateStdDataFile (file 1)" -Executable "desfire_create_std_data_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex,
    "--file-no", "1",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--file-size", "40",
    "--allow-existing"
)

Invoke-DemoCommand -Title "DES app: CreateValueFile (file 3)" -Executable "desfire_create_value_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex,
    "--file-no", "3",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--lower-limit", "-500",
    "--upper-limit", "500",
    "--limited-credit-value", "50",
    "--limited-credit-enabled",
    "--allow-existing"
)

Invoke-DemoCommand -Title "DES app: GetFileIDs" -Executable "desfire_get_file_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $desAppAuthArgs)

Invoke-DemoCommand -Title "DES app: Write+Read standard file (file 1)" -Executable "desfire_read_write_data_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--authenticate",
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex,
    "--file-no", "1",
    "--write-offset", "0",
    "--write-hex", "44 45 53 20 44 65 6D 6F 20 44 61 74 61"
)

Invoke-DemoCommand -Title "DES app: Credit 30 on value file" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--authenticate",
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex,
    "--file-no", "3",
    "--op", "credit",
    "--value", "30",
    "--commit",
    "--show-before",
    "--show-after"
)

Invoke-DemoCommand -Title "DES app: Debit 5 on value file" -Executable "desfire_value_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--authenticate",
    "--auth-mode", "legacy",
    "--auth-key-no", "0",
    "--auth-key-hex", $DesAppKeyHex,
    "--file-no", "3",
    "--op", "debit",
    "--value", "5",
    "--commit",
    "--show-before",
    "--show-after"
)

Invoke-DemoCommand -Title "DES app: GetFileSettings (file 3)" -Executable "desfire_get_file_settings_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedDesAid,
    "--file-no", "3"
) + $desAuthOnlyArgs)

# Stage 6: 2K3DES app file creation and usage.
Invoke-DemoCommand -Title "2K3DES app: CreateStdDataFile (file 1)" -Executable "desfire_create_std_data_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex,
    "--file-no", "1",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--file-size", "48",
    "--allow-existing"
)

Invoke-DemoCommand -Title "2K3DES app: CreateLinearRecordFile (file 4)" -Executable "desfire_create_linear_record_file_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex,
    "--file-no", "4",
    "--comm-mode", "enc",
    "--read-access", "key0",
    "--write-access", "key0",
    "--read-write-access", "key0",
    "--change-access", "key0",
    "--record-size", "16",
    "--max-records", "12",
    "--allow-existing"
)

Invoke-DemoCommand -Title "2K3DES app: GetFileIDs" -Executable "desfire_get_file_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $twoK3DesAppAuthArgs)

Invoke-DemoCommand -Title "2K3DES app: Write+Read standard file (file 1)" -Executable "desfire_read_write_data_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex,
    "--file-no", "1",
    "--write-offset", "0",
    "--write-hex", "32 4B 33 44 45 53 20 44 65 6D 6F 20 44 61 74 61"
)

Invoke-DemoCommand -Title "2K3DES app: Write and read one record (file 4)" -Executable "desfire_record_operations_example.exe" -Arguments @(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--authenticate",
    "--auth-mode", "iso",
    "--auth-key-no", "0",
    "--auth-key-hex", $TwoK3DesAppKeyHex,
    "--file-no", "4",
    "--write-offset", "0",
    "--write-hex", "90 91 92 93 94 95 96 97 98 99 9A 9B 9C 9D 9E 9F",
    "--commit",
    "--read-offset", "0",
    "--read-count", "1"
)

Invoke-DemoCommand -Title "2K3DES app: GetFileSettings (file 4)" -Executable "desfire_get_file_settings_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud",
    "--aid", $normalizedTwoK3DesAid,
    "--file-no", "4"
) + $twoK3DesAuthOnlyArgs)

# Stage 7: Final PICC checks and optional cleanup.
Invoke-DemoCommand -Title "FreeMemory (after demo)" -Executable "desfire_free_memory_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud",
    "--aid", "000000"
) + $piccAuthArgs)

Invoke-DemoCommand -Title "GetApplicationIDs (final)" -Executable "desfire_get_application_ids_example.exe" -Arguments (@(
    $ComPort,
    "--baud", "$Baud"
) + $piccAuthArgs)

if ($CleanupAtEnd) {
    Invoke-DemoCommand -Title "Cleanup: Delete AES app" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedAesAid,
        "--allow-missing"
    )

    Invoke-DemoCommand -Title "Cleanup: Delete 3K3DES app" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedThreeDesAid,
        "--allow-missing"
    )

    Invoke-DemoCommand -Title "Cleanup: Delete DES app" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedDesAid,
        "--allow-missing"
    )

    Invoke-DemoCommand -Title "Cleanup: Delete 2K3DES app" -Executable "desfire_delete_application_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--picc-auth-mode", $PiccAuthMode,
        "--picc-auth-key-no", "$PiccAuthKeyNo",
        "--picc-auth-key-hex", $PiccAuthKeyHex,
        "--app-aid", $normalizedTwoK3DesAid,
        "--allow-missing"
    )
}

if ($RunSetConfiguration) {
    Invoke-DemoCommand -Title "DANGEROUS: SetConfiguration (PICC mode flags)" -Executable "desfire_set_configuration_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--auth-mode", $PiccAuthMode,
        "--auth-key-no", "$PiccAuthKeyNo",
        "--auth-key-hex", $PiccAuthKeyHex,
        "--mode", "picc",
        "--disable-format", "0",
        "--enable-random-uid", "0",
        "--confirm-change"
    )
}

if ($RunFormatPicc) {
    Invoke-DemoCommand -Title "DANGEROUS: FormatPICC (ERASE CARD)" -Executable "desfire_format_picc_example.exe" -Arguments @(
        $ComPort,
        "--baud", "$Baud",
        "--aid", "000000",
        "--authenticate",
        "--auth-mode", $PiccAuthMode,
        "--auth-key-no", "$PiccAuthKeyNo",
        "--auth-key-hex", $PiccAuthKeyHex,
        "--confirm"
    )
}

Write-Host ""
Write-Host "Demo finished successfully." -ForegroundColor Green
