# DESFire Authenticate Example

This example performs:

1. `SelectApplication` (default PICC `000000`)
2. `Authenticate`

## Build

PowerShell:

```powershell
.\examples\desfire_authenticate\build_example.ps1
```

Batch:

```bat
examples\desfire_authenticate\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_authenticate_example.exe <COM_PORT> [options]
```

## Arguments

- `--baud <n>`: serial baud rate, default `115200`
- `--aid <hex6>`: application AID, default `000000` (PICC)
- `--auth-mode <legacy|iso|aes|des|2k3des|3k3des>`: auth mode, default `iso`
  - `des` maps to `legacy`
  - `2k3des` and `3k3des` map to `iso`
- `--auth-key-no <n>`: key number, default `0`
- `--auth-key-hex <hex>`: required key bytes
  - `legacy`: 8 or 16 bytes
  - `iso`: 8, 16, or 24 bytes
  - `aes`: 16 bytes

## Run

Authenticate PICC key 0 in ISO mode with all-zero 2K3DES key:

```powershell
.\build\examples\Debug\desfire_authenticate_example.exe COM5 `
  --aid 000000 `
  --auth-mode iso `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
```

Authenticate app `BADA55` key 0 in 3K3DES mode (alias):

```powershell
.\build\examples\Debug\desfire_authenticate_example.exe COM5 `
  --aid BADA55 `
  --auth-mode 3k3des `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88"
```

Authenticate app `BADA55` key 0 in AES mode:

```powershell
.\build\examples\Debug\desfire_authenticate_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```
