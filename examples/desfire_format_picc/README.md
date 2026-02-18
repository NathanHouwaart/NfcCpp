# DESFire FormatPICC Example

This example performs:

1. `SelectApplication(<aid>)` (default `000000`, PICC)
2. Optional `Authenticate`
3. `FormatPICC()`

Use this to erase the entire PICC content (all applications/files).

## Build

PowerShell:

```powershell
.\examples\desfire_format_picc\build_example.ps1
```

Batch:

```bat
examples\desfire_format_picc\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_format_picc_example.exe <COM_PORT> [options]
```

## Command Key and Purpose

- `FormatPICC` uses INS `0xFC`.
- Payload is empty.
- It formats (erases) the PICC and removes all applications/files.
- This is destructive and usually requires PICC master authentication.

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, e.g. `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Selected AID before format. For PICC format use `000000`. |
| `--authenticate` | No | off | Authenticate before `FormatPICC`. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Auth mode (`des` -> `legacy`, `2k3des/3k3des` -> `iso`). |
| `--auth-key-no <n>` | No | `0` | Auth key number. |
| `--auth-key-hex <hex>` | Cond. | - | Required when `--authenticate` is set. |
| `--confirm` | Yes | off | Required safety flag to execute destructive format. |

## Usage Example

Format PICC with AES authenticate:

```powershell
.\build\examples\Debug\desfire_format_picc_example.exe COM5 `
  --aid 000000 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --confirm
```

## Notes

- `FormatPICC` is PICC-level and should be run with selected AID `000000`.
- The operation is irreversible.
- If key settings require it, skipping authentication causes `PermissionDenied`/`AuthenticationError`.

