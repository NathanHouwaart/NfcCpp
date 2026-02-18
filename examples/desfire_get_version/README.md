# DESFire GetVersion Example

This example runs `GetVersion` and prints:

1. Raw version payload bytes (all frames concatenated)
2. Decoded EV1-style fields (hardware/software blocks, UID, batch, production data)

## Build

PowerShell:

```powershell
.\examples\desfire_get_version\build_example.ps1
```

Batch:

```bat
examples\desfire_get_version\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_get_version_example.exe <COM_PORT> [options]
```

## Arguments

- `<COM_PORT>`: required serial port, for example `COM5`
- `--baud <n>`: serial baud rate, default `115200`

## Run

```powershell
.\build\examples\Debug\desfire_get_version_example.exe COM5
```

With explicit baud:

```powershell
.\build\examples\Debug\desfire_get_version_example.exe COM5 --baud 115200
```

## Notes

- `GetVersion` usually requires no authentication.
- The command may return multiple frames internally; this example handles that automatically.
- Some card generations can include slightly different payload details; raw bytes are always printed.

