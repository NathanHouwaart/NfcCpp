# DESFire GetFileIDs Command Guide

## Purpose

`GetFileIDs` retrieves all active file numbers in the currently selected application.

- Command code: `0x6F`
- Scope: selected application
- Security: depends on app key settings (may require app master-key auth)

## Response Format

The payload is a byte array of file IDs:

- each entry is one file number byte
- maximum is 32 files per application

## APDU/Frame Notes

`GetFileIDs` is usually a single-frame command.

In authenticated sessions, some cards/transports can append MAC/CMAC bytes to the final response.
This implementation trims a trailing 8-byte (or 4-byte fallback) suffix if needed to keep payload within 32 bytes.

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/GetFileIdsCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/GetFileIdsCommand.cpp`
- Card helper method: `DesfireCard::getFileIds()` in
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

## Typical Flow

1. `SelectApplication(<aid>)`
2. Optional `Authenticate(<key>)`
3. `GetFileIDs`

## Example

Use:

`examples/desfire_get_file_ids/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_get_file_ids_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

