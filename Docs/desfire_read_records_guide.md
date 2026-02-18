# DESFire ReadRecords Command Guide

## Purpose

`ReadRecords` reads records from a linear or cyclic record file in the currently selected application.

- Command key (INS): `0xBB`
- Security: file/application access rights apply

This project exposes the command through:

- `ReadRecordsCommand`
- `DesfireCard::readRecords(...)`

## Payload Layout

`ReadRecords` uses a 7-byte payload:

1. `FileNo` (1 byte)
2. `RecordOffset` (3 bytes, LSB first)
3. `RecordCount` (3 bytes, LSB first)

These fields are record-based.

## Parameters

- `fileNo`: target file number (`0..31`)
- `recordOffset`: index of first record (`0..0xFFFFFF`)
- `recordCount`: number of records to read (`0..0xFFFFFF`)

`recordCount = 0` means "read all records from `recordOffset`".

Returned payload data is raw bytes for all returned records (`recordCount * recordSize` bytes).

## Preconditions

Before reading:

1. `SelectApplication(target AID)`
2. Authenticate if file access rights require it
3. Ensure the target file is a record file and has available records

Typical errors:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`
- `BoundaryError (0xBE)` when requested range exceeds available records

## Implementation Notes

- Command source:
  - `Include/Nfc/Desfire/Commands/ReadRecordsCommand.h`
  - `Src/Nfc/Desfire/Commands/ReadRecordsCommand.cpp`
- Card helper method:
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

`DesfireCard::readRecords(...)` first reads file settings to:

- confirm record-file type
- resolve effective record count (`0 => all`)
- validate bounds against `currentRecords`
- compute expected byte length (`recordCount * recordSize`) for response validation

In authenticated sessions, trailing CMAC bytes in final responses are trimmed using expected length.

## Usage from Example

See:

- `examples/desfire_record_operations/main.cpp`
- `examples/desfire_record_operations/README.md`

Read one record:

```powershell
.\build\examples\Debug\desfire_record_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --read-offset 0 `
  --read-count 1
```
