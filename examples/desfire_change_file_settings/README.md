# DESFire ChangeFileSettings Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. Optional `GetFileSettings(<fileNo>)` (before)
4. `ChangeFileSettings(<fileNo>, <new comm>, <access rights>)`
5. Optional `GetFileSettings(<fileNo>)` (after)

`ChangeFileSettings` updates the communication mode and access-right nibbles of an existing file.

## Build

PowerShell:

```powershell
.\examples\desfire_change_file_settings\build_example.ps1
```

Batch:

```bat
examples\desfire_change_file_settings\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_change_file_settings_example.exe <COM_PORT> [options]
```

## Command Key and Purpose

- `ChangeFileSettings` uses INS `0x5F`.
- Logical payload fields:
  - `fileNo` (target file)
  - `communication settings` (`0x00` plain, `0x01` MAC, `0x03` enciphered)
  - access-right bytes (`RW/CAR` and `R/W`)
- Use it to change who can read/write/change a file and how file-level data commands are protected.

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, e.g. `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Selected application AID. |
| `--file-no <n>` | Yes | - | File number (`0..31`). |
| `--new-comm-mode <plain\|mac\|enc\|0x00\|0x01\|0x03>` | Yes | - | New file communication mode. |
| `--read-access <n\|keyN\|free\|never>` | Yes | - | New read access nibble (`0x0..0xF`). |
| `--write-access <n\|keyN\|free\|never>` | Yes | - | New write access nibble (`0x0..0xF`). |
| `--read-write-access <n\|keyN\|free\|never>` | Yes | - | New read+write access nibble (`0x0..0xF`). |
| `--change-access <n\|keyN\|free\|never>` | Yes | - | New change-rights nibble (`0x0..0xF`). |
| `--command-comm-mode <auto\|plain\|mac\|enc\|0x00\|0x01\|0x03>` | No | `auto` | Protection mode for this command itself. |
| `--authenticate` | No | off | Authenticate before command. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Auth mode (`des`->`legacy`, `2k3des/3k3des`->`iso`). |
| `--session-key-type <des\|2k3des\|3k3des\|aes>` | No | auto | Optional session-cipher override. |
| `--auth-key-no <n>` | No | `0` | Auth key number. |
| `--auth-key-hex <hex>` | Cond. | - | Required when `--authenticate` is set. |
| `--show-before` | No | off | Read and print file settings before change. |
| `--show-after` | No | off | Read and print file settings after change. |
| `--confirm-change` | No | off | Actually execute `ChangeFileSettings`; without this, example is dry-run. |

## Usage Example

```powershell
.\build\examples\Debug\desfire_change_file_settings_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --new-comm-mode enc `
  --read-access key0 `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --command-comm-mode auto `
  --show-before `
  --show-after `
  --confirm-change
```

## Notes

- `ChangeFileSettings` is application-level: select the correct AID first.
- Wrong access-right values can lock you out from a file. Use `--show-before` and `--show-after` to verify.
- `mac` command-comm mode is currently not implemented in this command path.

