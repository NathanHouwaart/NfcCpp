# DESFire GetFileSettings Command Guide

## Purpose

`GetFileSettings` retrieves metadata for one file in the selected application.

- Command code: `0xF5`
- Scope: selected application + file number
- Security: depends on application key settings

## Request

Payload:

- `FileNo` (1 byte)

## Response Layout

Common header (first 4 bytes):

1. `FileType`
2. `CommunicationSettings`
3. `AccessRights1` (`[RW|CAR]`)
4. `AccessRights2` (`[R|W]`)

### FileType-specific payload

- `0x00` StandardData / `0x01` BackupData:
  - `FileSize` (3 bytes, LSB first)
- `0x02` Value:
  - `LowerLimit` (4 bytes, LSB first)
  - `UpperLimit` (4 bytes, LSB first)
  - `LimitedCreditValue` (4 bytes, LSB first)
  - `Flags` (1 byte)
    - bit0: limited credit enabled
    - bit1: free get-value enabled (if present on card flavor)
- `0x03` LinearRecord / `0x04` CyclicRecord:
  - `RecordSize` (3 bytes, LSB first)
  - `MaxRecords` (3 bytes, LSB first)
  - `CurrentRecords` (3 bytes, LSB first)

## Access Rights Decode

Access nibble values:

- `0x0..0xD`: key 0..13
- `0x0E`: free
- `0x0F`: never

## APDU/Frame Notes

`GetFileSettings` is usually single-frame.

In authenticated sessions, a trailing MAC/CMAC can appear in response data with some stacks/cards.
This implementation retries parsing after trimming 8 bytes (or 4 bytes fallback).

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/GetFileSettingsCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/GetFileSettingsCommand.cpp`
- Card helper method: `DesfireCard::getFileSettings(uint8_t)` in
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

## Typical Flow

1. `SelectApplication(<aid>)`
2. Optional `Authenticate(<key>)`
3. `GetFileSettings(fileNo)`

## Example

Use:

`examples/desfire_get_file_settings/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_get_file_settings_example.exe COM5 `
  --aid BADA55 `
  --file-no 1 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

