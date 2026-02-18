param(
    [string]$BuildDir = "build",
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$BuildPath = Join-Path $RepoRoot $BuildDir

Write-Host "Configuring CMake in $BuildPath ..."
cmake -S $RepoRoot -B $BuildPath -DNFCCPP_BUILD_EXAMPLES=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Building desfire_change_file_settings_example ($Config) ..."
cmake --build $BuildPath --target desfire_change_file_settings_example --config $Config
exit $LASTEXITCODE

