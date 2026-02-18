# DESFire GetFileIDs Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. `GetFileIDs`
4. Print all file IDs in the selected application

## Build

PowerShell:

```powershell
.\examples\desfire_get_file_ids\build_example.ps1
```

Batch:

```bat
examples\desfire_get_file_ids\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_get_file_ids_example.exe <COM_PORT> [options]
```

## Arguments

- `<COM_PORT>`: required serial port, for example `COM5`
- `--baud <n>`: serial baud rate, default `115200`
- `--aid <hex6>`: target AID, default `000000`
- `--authenticate`: authenticate before querying file IDs
- `--auth-mode <legacy|iso|aes|des|2k3des|3k3des>`: auth mode, default `iso`
  - `des` maps to `legacy`
  - `2k3des` and `3k3des` map to `iso`
- `--auth-key-no <n>`: key number, default `0`
- `--auth-key-hex <hex>`: required when `--authenticate` is set

## Run

Get file IDs in app `BADA55` after AES authenticate:

```powershell
.\build\examples\Debug\desfire_get_file_ids_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

Get file IDs in PICC context without authenticate:

```powershell
.\build\examples\Debug\desfire_get_file_ids_example.exe COM5 `
  --aid 000000
```

## Notes

- `GetFileIDs` is application scoped. Select the correct AID first.
- If permissions require it, run authenticate before this command.

