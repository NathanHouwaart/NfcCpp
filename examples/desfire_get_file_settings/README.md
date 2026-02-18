# DESFire GetFileSettings Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. `GetFileSettings(<fileNo>)`
4. Print decoded file metadata

## Build

PowerShell:

```powershell
.\examples\desfire_get_file_settings\build_example.ps1
```

Batch:

```bat
examples\desfire_get_file_settings\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_get_file_settings_example.exe <COM_PORT> [options]
```

## Arguments

- `<COM_PORT>`: required serial port, for example `COM5`
- `--baud <n>`: serial baud rate, default `115200`
- `--aid <hex6>`: target AID, default `000000`
- `--file-no <n>`: file number, default `0`
- `--authenticate`: authenticate before querying file settings
- `--auth-mode <legacy|iso|aes|des|2k3des|3k3des>`: auth mode, default `iso`
  - `des` maps to `legacy`
  - `2k3des` and `3k3des` map to `iso`
- `--auth-key-no <n>`: key number, default `0`
- `--auth-key-hex <hex>`: required when `--authenticate` is set

## Run

Read file settings for file `1` in app `BADA55` after AES authenticate:

```powershell
.\build\examples\Debug\desfire_get_file_settings_example.exe COM5 `
  --aid BADA55 `
  --file-no 1 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

Read file settings for file `0` without authenticate:

```powershell
.\build\examples\Debug\desfire_get_file_settings_example.exe COM5 `
  --aid BADA55 `
  --file-no 0
```

## Notes

- `GetFileSettings` is application scoped. Select the correct AID first.
- Depending on key settings, reading file settings may require authentication.
- The example decodes:
  - common fields: file type, comm mode, access rights
  - standard/backup fields: file size
  - value fields: limits and limited-credit flags
  - record fields: record geometry

