# DESFire DeleteFile Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. `DeleteFile(<fileNo>)`

Use this to remove one file from the currently selected DESFire application.

## Build

PowerShell:

```powershell
.\examples\desfire_delete_file\build_example.ps1
```

Batch:

```bat
examples\desfire_delete_file\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_delete_file_example.exe <COM_PORT> [options]
```

## Command Key and Purpose

- `DeleteFile` uses INS `0xDF`.
- Payload is one byte: target `fileNo`.
- It deletes one file (standard/backup/value/record) from the selected application.

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, e.g. `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Target application AID (3 bytes). |
| `--file-no <n>` | Yes | - | File number to delete (`0..31`). |
| `--authenticate` | No | off | Authenticate before delete. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Auth mode (`des` -> `legacy`, `2k3des/3k3des` -> `iso`). |
| `--auth-key-no <n>` | No | `0` | Auth key number. |
| `--auth-key-hex <hex>` | Cond. | - | Required when `--authenticate` is set. |
| `--allow-missing` | No | off | Treat `FileNotFound (0xF0)` as success. |

## Usage Example

Delete file `5` in app `BADA55`:

```powershell
.\build\examples\Debug\desfire_delete_file_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5
```

## Notes

- Access permissions are enforced by the card.
- If delete rights require authentication and you skip it, you can get `PermissionDenied` or `AuthenticationError`.
- Deleting a non-existing file returns `FileNotFound` unless `--allow-missing` is used.

