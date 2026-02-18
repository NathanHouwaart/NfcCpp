# DESFire ChangeKeySettings Example

This example performs:

1. `SelectApplication` (default `000000` PICC)
2. `Authenticate`
3. `ChangeKeySettings` (when `--confirm-change` is provided)

`ChangeKeySettings` writes **KeySettings1** (one byte) for the selected PICC/application.

## Build

PowerShell:

```powershell
.\examples\desfire_change_key_settings\build_example.ps1
```

Batch:

```bat
examples\desfire_change_key_settings\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_change_key_settings_example.exe <COM_PORT> [options]
```

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, for example `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Selected AID (3 bytes). Use `000000` for PICC settings. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Authentication mode. Aliases: `des`->`legacy`, `2k3des/3k3des`->`iso`. |
| `--session-key-type <des\|2k3des\|3k3des\|aes>` | No | auto | Optional session cipher override for encryption framing. |
| `--auth-key-no <n>` | No | `0` | Authentication key number. |
| `--auth-key-hex <hex>` | Yes | - | Authentication key bytes. |
| `--key-settings <n>` | Cond. | - | Base KeySettings1 byte (hex/dec, `0..255`). |
| `--allow-change-mk <0\|1>` | No | - | Override bit0. |
| `--allow-listing-without-mk <0\|1>` | No | - | Override bit1 (PICC relevant). |
| `--allow-create-delete-without-mk <0\|1>` | No | - | Override bit2 (PICC relevant). |
| `--configuration-changeable <0\|1>` | No | - | Override bit3. |
| `--change-key-access <mk\|keyN\|same\|frozen\|0..15>` | No | - | Override upper nibble (bits 4..7). |
| `--confirm-change` | No | off | Actually send `ChangeKeySettings`; without this, the example is dry-run only. |

You must provide either:

- `--key-settings`, or
- one or more helper overrides (`--allow-*`, `--configuration-changeable`, `--change-key-access`)

## KeySettings1 Bit Layout

- Bit 0 (`0x01`): master key may be changed
- Bit 1 (`0x02`): `GetApplicationIDs` / `GetKeySettings` allowed without PICC master key auth
- Bit 2 (`0x04`): `CreateApplication` / `DeleteApplication` allowed without PICC master key auth
- Bit 3 (`0x08`): configuration/key-settings may be changed later
- Bits 4..7: rule for which key must authenticate to change keys
  - `0x0`: master key
  - `0x1..0xD`: key1..key13
  - `0xE`: same key being changed
  - `0xF`: all key changes frozen

Common values:

- `0x0F`: factory-default style lower nibble enabled, change-key access via master key
- `0xF0`: key changes frozen

## Run Examples

Set PICC key settings directly to `0x0F`:

```powershell
.\build\examples\Debug\desfire_change_key_settings_example.exe COM5 `
  --aid 000000 `
  --auth-mode legacy `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00" `
  --key-settings 0x0F `
  --confirm-change
```

Build value via helpers (base `0x00`, enable all low bits, require MK for key changes):

```powershell
.\build\examples\Debug\desfire_change_key_settings_example.exe COM5 `
  --aid 000000 `
  --auth-mode legacy `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00" `
  --allow-change-mk 1 `
  --allow-listing-without-mk 1 `
  --allow-create-delete-without-mk 1 `
  --configuration-changeable 1 `
  --change-key-access mk `
  --confirm-change
```

Application-level update with AES auth:

```powershell
.\build\examples\Debug\desfire_change_key_settings_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --key-settings 0x1E `
  --confirm-change
```

## Notes

- This command can lock you out if you set restrictive bits incorrectly.
- Always validate auth first (dry run) and keep known-good key material before changing settings.
- For PowerShell, quote spaced hex strings, for example `"00 00 ..."`; unquoted bytes are parsed as separate arguments.
