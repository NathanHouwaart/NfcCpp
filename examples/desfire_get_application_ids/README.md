# DESFire GetApplicationIDs Example

This example performs:

1. `SelectApplication(000000)` to select PICC
2. Optional PICC authenticate
3. `GetApplicationIDs`
4. Print returned AIDs

## Build

PowerShell:

```powershell
.\examples\desfire_get_application_ids\build_example.ps1
```

Batch:

```bat
examples\desfire_get_application_ids\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_get_application_ids_example.exe <COM_PORT> [options]
```

## Arguments

- `<COM_PORT>`: required serial port, for example `COM5`
- `--baud <n>`: serial baud rate, default `115200`
- `--authenticate`: authenticate at PICC level before listing apps
- `--auth-mode <legacy|iso|aes|des|2k3des|3k3des>`: auth mode, default `iso`
  - `des` maps to `legacy`
  - `2k3des` and `3k3des` map to `iso`
- `--auth-key-no <n>`: key number, default `0`
- `--auth-key-hex <hex>`: required when `--authenticate` is set

## Run

List applications without PICC authenticate:

```powershell
.\build\examples\Debug\desfire_get_application_ids_example.exe COM5
```

List applications with PICC authenticate (ISO / 2K3DES all-zero key):

```powershell
.\build\examples\Debug\desfire_get_application_ids_example.exe COM5 `
  --authenticate `
  --auth-mode iso `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
```

List applications with PICC authenticate (AES):

```powershell
.\build\examples\Debug\desfire_get_application_ids_example.exe COM5 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

## Notes

- Some cards/key settings allow listing apps without authentication; others require PICC auth.
- Returned AIDs are printed in the same byte order as delivered by the card.

