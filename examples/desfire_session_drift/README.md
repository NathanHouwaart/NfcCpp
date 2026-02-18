# DESFire Session Drift Example

This example is a long-session stress runner for IV/CMAC drift detection.

It creates/uses 4 applications (by default):

- AES (`A1A551`)
- 3K3DES (`A1A552`)
- DES (`A1A553`)
- 2K3DES (`A1A554`)

For each app it ensures file setup and then runs mixed operations across all major file types:

- Standard data file (`WriteData` + `ReadData`)
- Backup data file (`WriteData` + `CommitTransaction` + `ReadData`)
- Value file (`GetValue` + `Credit` + `CommitTransaction` + `GetValue`)
- Linear record file (`WriteRecord` + `CommitTransaction` + `ReadRecords`)
- Cyclic record file (`WriteRecord` + `CommitTransaction` + `ReadRecords`)

## Why this exists

Short command demos often re-authenticate per command and hide session progression bugs.

This runner supports:

- `drift` mode: authenticate once per app workload
- `baseline` mode: re-authenticate before every operation

Use the same workload in both modes and compare behavior.

## Build

PowerShell:

```powershell
.\examples\desfire_session_drift\build_example.ps1
```

Batch:

```bat
examples\desfire_session_drift\build_example.bat
```

## Usage

```powershell
.\build\examples\Debug\desfire_session_drift_example.exe <COM_PORT> [options]
```

### Main options

- `--mode <drift|baseline>`: default `drift`
- `--repeat <n>`: repeats workload per app, default `3`
- `--trace-iv`: print session state (`authenticated`, `authScheme`, `keyNo`, `iv`) around each step
- `--chunk-size <n>`: chunk size passed to IO operations, default `0` (command default)
- `--recreate-apps`: delete test apps before creating
- `--allow-existing`: continue when create returns `DuplicateError`

### PICC auth options

- `--picc-auth-mode <legacy|iso|aes>`: default `legacy`
- `--picc-auth-key-no <n>`: default `0`
- `--picc-auth-key-hex <hex>`: default all-zero key for selected mode

### App config options

- `--app-key-settings <n>`: default `0x0F`
- `--app-key-count <n>`: default `2`
- `--aid-aes <hex6>`: default `A1A551`
- `--aid-3k3des <hex6>`: default `A1A552`
- `--aid-des <hex6>`: default `A1A553`
- `--aid-2k3des <hex6>`: default `A1A554`

## Example runs

Drift mode (single auth per app workload):

```powershell
.\build\examples\Debug\desfire_session_drift_example.exe COM5 `
  --mode drift `
  --repeat 5 `
  --recreate-apps `
  --picc-auth-mode legacy `
  --picc-auth-key-no 0 `
  --picc-auth-key-hex "00 00 00 00 00 00 00 00"
```

Baseline mode (re-auth before every operation):

```powershell
.\build\examples\Debug\desfire_session_drift_example.exe COM5 `
  --mode baseline `
  --repeat 5 `
  --recreate-apps `
  --picc-auth-mode legacy `
  --picc-auth-key-no 0 `
  --picc-auth-key-hex "00 00 00 00 00 00 00 00"
```

With IV tracing:

```powershell
.\build\examples\Debug\desfire_session_drift_example.exe COM5 `
  --mode drift `
  --repeat 2 `
  --trace-iv `
  --allow-existing `
  --picc-auth-mode legacy `
  --picc-auth-key-no 0 `
  --picc-auth-key-hex "00 00 00 00 00 00 00 00"
```

## Notes

- `--recreate-apps` is the most deterministic mode for regression testing.
- `--allow-existing` is useful for iterative runs without deleting data.
- The default app keys match the scripted demo keys used in this repository.
