# DESFire SetConfiguration Example

This example performs:

1. `SelectApplication(000000)` (PICC level)
2. `Authenticate` with PICC master key
3. `SetConfiguration` in one of two modes:
   - PICC config flags (`subcommand 0x00`)
   - ATS bytes (`subcommand 0x01`)

## Build

PowerShell:

```powershell
.\examples\desfire_set_configuration\build_example.ps1
```

Batch:

```bat
examples\desfire_set_configuration\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_set_configuration_example.exe <COM_PORT> [options]
```

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port, for example `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Authentication mode for PICC auth. |
| `--session-key-type <des\|2k3des\|3k3des\|aes>` | No | auto | Optional explicit session cipher type. |
| `--auth-key-no <n>` | No | `0` | PICC auth key number. |
| `--auth-key-hex <hex>` | Yes | - | PICC auth key bytes. |
| `--mode <picc\|ats>` | Yes | - | Select SetConfiguration payload type. |
| `--config-byte <n>` | Cond. | - | Base config byte for `mode=picc`. |
| `--disable-format <0\|1>` | No | - | Override bit0 in PICC config byte. |
| `--enable-random-uid <0\|1>` | No | - | Override bit1 in PICC config byte. |
| `--ats-hex <hex>` | Cond. | - | ATS payload for `mode=ats`. If TL is missing, the tool prepends it. |
| `--confirm-change` | No | off | Actually sends SetConfiguration. |

For `mode=picc`, provide at least one of:

- `--config-byte`
- `--disable-format`
- `--enable-random-uid`

For `mode=ats`, `--ats-hex` is required.

## Run Examples

Enable random UID and disable format (PICC flags):

```powershell
.\build\examples\Debug\desfire_set_configuration_example.exe COM5 `
  --auth-mode legacy `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00" `
  --mode picc `
  --disable-format 1 `
  --enable-random-uid 1 `
  --confirm-change
```

Set ATS:

```powershell
.\build\examples\Debug\desfire_set_configuration_example.exe COM5 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --mode ats `
  --ats-hex "06 75 77 81 02 80" `
  --confirm-change
```

## Important Safety Notes

- `SetConfiguration` changes PICC behavior, not application key policy.
- `disable-format` and random UID behavior can be hard or impossible to revert on some cards.
- Test with dry-run first (omit `--confirm-change`) to verify auth and parsing.

## SetConfiguration vs ChangeKeySettings

- `ChangeKeySettings` (`0x54`) updates **KeySettings1** access policy bits:
  who may list/create/delete/change keys/files.
- `SetConfiguration` (`0x5C`) updates **PICC runtime/configuration behavior**:
  random UID flag, format behavior, ATS bytes.

Use `ChangeKeySettings` for access policy, and `SetConfiguration` for PICC behavior.
