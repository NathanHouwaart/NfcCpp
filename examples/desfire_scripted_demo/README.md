# DESFire Scripted Demo (No C++)

This example is a PowerShell-only orchestrator that chains the existing DESFire example executables into one end-to-end card demo.

- Script: `run_demo.ps1`
- No C++ source is added in this example folder.

The demo:
- uses PICC authentication only (it does **not** change the PICC key),
- creates four applications with different key families (`aes`, `3k3des`, `des`, `2k3des`),
- rotates each app key to a custom value,
- creates and uses data/value/record files,
- performs read/write/value/record operations,
- checks key/file settings and file IDs,
- demonstrates clear/delete/commit flows.

## Prerequisites

1. Build the project so all example executables exist under `build/examples/Debug`.
2. Connect PN532 and know your PICC authentication credentials.
3. PowerShell 5+.

## Quick Run

```powershell
.\examples\desfire_scripted_demo\run_demo.ps1 `
  -ComPort COM5 `
  -PiccAuthMode iso `
  -PiccAuthKeyNo 0 `
  -PiccAuthKeyHex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
```

## Dry Run (Print Commands Only)

```powershell
.\examples\desfire_scripted_demo\run_demo.ps1 `
  -ComPort COM5 `
  -PiccAuthMode iso `
  -PiccAuthKeyNo 0 `
  -PiccAuthKeyHex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" `
  -DryRun
```

## Important Parameters

- `-ComPort` (required): e.g. `COM5`.
- `-PiccAuthMode` / `-PiccAuthKeyNo` / `-PiccAuthKeyHex` (required key material): used only for PICC-auth commands.
- `-AesAppAid` / `-AesAppKeyHex`: AES demo app and its final key.
- `-ThreeDesAppAid` / `-ThreeDesAppKeyHex`: 3K3DES demo app and its final key.
- `-DesAppAid` / `-DesAppKeyHex`: DES demo app and its final key.
- `-TwoK3DesAppAid` / `-TwoK3DesAppKeyHex`: 2K3DES demo app and its final key.
- `-SkipInitialCleanup`: do not delete demo AIDs before creation.
- `-CleanupAtEnd`: delete demo apps after demo completes.

Key policy in this script:
- App target keys (`AesAppKeyHex`, `ThreeDesAppKeyHex`, `DesAppKeyHex`, `TwoK3DesAppKeyHex`) are validated to be **non-zero**.
- App creation still authenticates with the default all-zero initial key, then rotates to your non-zero demo key.

Dangerous (opt-in) options:
- `-RunSetConfiguration`
- `-RunFormatPicc`
- `-ConfirmDangerousPiccOperations` (required if either dangerous option is used)

## Safety Notes

- The default flow does **not** change PICC keys.
- The default flow does **not** run `FormatPICC` or `SetConfiguration`.
- `FormatPICC` erases the card; it is gated by explicit flags.

## Covered Command Families

The scripted flow exercises these implemented command examples:

- `GetVersion`, `GetApplicationIDs`, `GetCardUID`, `FreeMemory`
- `CreateApplication`, `DeleteApplication`
- `GetKeySettings` + `GetKeyVersion` (via `desfire_get_key_info_example`)
- `ChangeKey` (via `desfire_auth_changekey_example`)
- `ChangeKeySettings`
- `CreateStdDataFile`, `CreateBackupDataFile`, `CreateValueFile`, `CreateLinearRecordFile`, `CreateCyclicRecordFile`
- `GetFileIDs`, `GetFileSettings`, `ChangeFileSettings`
- `ReadData`, `WriteData` (via `desfire_read_write_data_example`)
- `GetValue`, `Credit`, `Debit`, `LimitedCredit` (via `desfire_value_operations_example`)
- `ReadRecords`, `WriteRecord` (via `desfire_record_operations_example`)
- `ClearRecordFile`, `DeleteFile`, `CommitTransaction`

Note: `AbortTransaction` is not included because there is currently no dedicated standalone example executable for it.

## Step-by-Step Plan

See `examples/desfire_scripted_demo/STEPS.md` for the detailed step order and intent.
