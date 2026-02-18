# DESFire GetCardUID Example

This example performs:

1. `SelectApplication(000000)` to select PICC
2. PICC `Authenticate`
3. `GetCardUID`

## Build

PowerShell:

```powershell
.\examples\desfire_get_card_uid\build_example.ps1
```

Batch:

```bat
examples\desfire_get_card_uid\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_get_card_uid_example.exe <COM_PORT> [options]
```

## Arguments

- `<COM_PORT>`: required serial port, for example `COM5`
- `--baud <n>`: serial baud rate, default `115200`
- `--auth-mode <legacy|iso|aes|des|2k3des|3k3des>`: auth mode, default `iso`
  - `des` maps to `legacy`
  - `2k3des` and `3k3des` map to `iso`
- `--auth-key-no <n>`: key number, default `0`
- `--auth-key-hex <hex>`: required key bytes

## Run

PICC key 0, legacy DES all-zero key:

```powershell
.\build\examples\Debug\desfire_get_card_uid_example.exe COM5 `
  --auth-mode legacy `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00"
```

PICC key 0, ISO 2K3DES all-zero key:

```powershell
.\build\examples\Debug\desfire_get_card_uid_example.exe COM5 `
  --auth-mode iso `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
```

PICC key 0, AES key:

```powershell
.\build\examples\Debug\desfire_get_card_uid_example.exe COM5 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

## Notes

- `GetCardUID` generally requires successful PICC authentication.
- The command parser attempts both encrypted-response decoding and plain fallback parsing.

