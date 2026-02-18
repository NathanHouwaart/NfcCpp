# DESFire DeleteApplication Example

This example performs:

1. `SelectApplication(000000)` to select PICC.
2. `Authenticate` with the PICC master key.
3. `DeleteApplication(<target aid>)`.

## Build

PowerShell:

```powershell
.\examples\desfire_delete_application\build_example.ps1
```

Batch:

```bat
examples\desfire_delete_application\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_delete_application_example.exe <COM_PORT> [options]
```

## Arguments

- `--baud <n>`: serial baud rate, default `115200`
- `--picc-auth-mode <legacy|iso|aes>`: PICC auth mode, default `iso`
- `--picc-auth-key-no <n>`: PICC key number, default `0`
- `--picc-auth-key-hex <hex>`: required PICC auth key bytes
- `--app-aid <hex6>`: required application AID (3 bytes)
- `--allow-missing`: continue successfully if app does not exist

## Run

Delete app `BADA55` with PICC master key DES all `00`:

```powershell
.\build\examples\Debug\desfire_delete_application_example.exe COM5 `
  --picc-auth-mode legacy `
  --picc-auth-key-no 0 `
  --picc-auth-key-hex "00 00 00 00 00 00 00 00" `
  --app-aid BADA55 `
  --allow-missing
```

## Notes

- Deleting an app is a PICC-level operation. Select PICC and authenticate PICC first.
- If PICC settings do not permit create/delete without MK auth, missing authentication gives `PermissionDenied` or `AuthenticationError`.
- The command itself only needs AID; application key bytes are not required.

