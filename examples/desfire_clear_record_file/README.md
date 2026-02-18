# DESFire ClearRecordFile Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. `ClearRecordFile(<fileNo>)`
4. Optional `CommitTransaction()`

Use this to reset a linear or cyclic record file to an empty state.

## Build

PowerShell:

```powershell
.\examples\desfire_clear_record_file\build_example.ps1
```

Batch:

```bat
examples\desfire_clear_record_file\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_clear_record_file_example.exe <COM_PORT> [options]
```

## Command Key and Purpose

- `ClearRecordFile` uses INS `0xEB`.
- Payload is one byte: target `fileNo`.
- It clears record contents of a linear/cyclic record file.
- On DESFire this is transactional, so use `CommitTransaction (0xC7)` to persist.

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, e.g. `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Target application AID (3 bytes). |
| `--file-no <n>` | Yes | - | Record file number to clear (`0..31`). |
| `--authenticate` | No | off | Authenticate before clear. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Auth mode (`des` -> `legacy`, `2k3des/3k3des` -> `iso`). |
| `--auth-key-no <n>` | No | `0` | Auth key number. |
| `--auth-key-hex <hex>` | Cond. | - | Required when `--authenticate` is set. |
| `--commit` | No | off | Run `CommitTransaction` after clear. |
| `--allow-no-changes` | No | off | Treat commit `NoChanges (0x0C)` as success. |
| `--allow-missing` | No | off | Treat clear `FileNotFound (0xF0)` as success. |

## Usage Example

Clear record file `5` in app `BADA55` and commit:

```powershell
.\build\examples\Debug\desfire_clear_record_file_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --commit
```

## Notes

- Access permissions are enforced by the card.
- `ClearRecordFile` can fail with `PermissionDenied` or `AuthenticationError` when rights are insufficient.
- Without `--commit`, pending clear changes may not be persisted.

