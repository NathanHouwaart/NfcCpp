# DESFire GetKeySettings + GetKeyVersion Example

This example performs:

1. `SelectApplication` (default `000000` PICC)
2. Optional `Authenticate`
3. `GetKeySettings`
4. `GetKeyVersion` for one key number

## Build

PowerShell:

```powershell
.\examples\desfire_get_key_info\build_example.ps1
```

Batch:

```bat
examples\desfire_get_key_info\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_get_key_info_example.exe <COM_PORT> [options]
```

## Arguments

- `<COM_PORT>`: required serial port, for example `COM5`
- `--baud <n>`: serial baud rate, default `115200`
- `--aid <hex6>`: application AID, default `000000` (PICC)
- `--authenticate`: run authentication before key queries
- `--auth-mode <legacy|iso|aes|des|2k3des|3k3des>`: auth mode, default `iso`
  - `des` maps to `legacy`
  - `2k3des` and `3k3des` map to `iso`
- `--auth-key-no <n>`: auth key number, default `0`
- `--auth-key-hex <hex>`: required when `--authenticate` is used
- `--query-key-no <n>`: key number passed to `GetKeyVersion`, default `0`

## Run

PICC, with authenticate:

```powershell
.\build\examples\Debug\desfire_get_key_info_example.exe COM5 `
  --aid 000000 `
  --authenticate `
  --auth-mode legacy `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00" `
  --query-key-no 0
```

Application `BADA55`, no authenticate:

```powershell
.\build\examples\Debug\desfire_get_key_info_example.exe COM5 `
  --aid BADA55 `
  --query-key-no 0
```

## Notes

- `KeySettings2` encodes:
  - low nibble: key count
  - high type flags: `0x00` DES/2K3DES, `0x40` 3K3DES, `0x80` AES
- Some card/key settings require authentication before reading key metadata.

