# DESFire WriteRecord Command Guide

## Purpose

`WriteRecord` writes records to a linear or cyclic record file in the currently selected application.

- Command key (INS): `0x3B`
- Security: file/application access rights apply

This project exposes the command through:

- `WriteRecordCommand`
- `DesfireCard::writeRecord(...)`

## Payload Layout

`WriteRecord` uses this payload:

1. `FileNo` (1 byte)
2. `ByteOffset` (3 bytes, LSB first)
3. `ByteLength` (3 bytes, LSB first)
4. `Data` (`ByteLength` bytes)

In this project, API inputs are already byte-based and map directly to these on-wire fields.

## Parameters

- `fileNo`: target file number (`0..31`)
- `offset`: byte offset within the record under construction
- `data`: raw record payload bytes

Boundary rule: `offset + data.size() <= recordSize`.

## Transaction Behavior

Record file writes are transactional on DESFire.
After `WriteRecord`, call `CommitTransaction` to persist pending changes.

This repository exposes:

- `DesfireCard::commitTransaction()` (`INS 0xC7`)

## Preconditions

Before writing:

1. `SelectApplication(target AID)`
2. Authenticate if file access rights require it
3. Ensure target file is a linear/cyclic record file
4. Ensure `offset + data-size` does not exceed `recordSize`

Typical errors:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`
- `LengthError (0x7E)` for invalid payload framing/size
- `BoundaryError (0xBE)` for invalid write range within a record

## Implementation Notes

- Command source:
  - `Include/Nfc/Desfire/Commands/WriteRecordCommand.h`
  - `Src/Nfc/Desfire/Commands/WriteRecordCommand.cpp`
- Card helper method:
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

`DesfireCard::writeRecord(...)` reads file settings first to validate `recordSize` boundaries.
Chunks are split by byte budget, and each frame is encoded with byte `offset/length`.

## Usage from Example

See:

- `examples/desfire_record_operations/main.cpp`
- `examples/desfire_record_operations/README.md`

Write one 16-byte record and commit:

```powershell
.\build\examples\Debug\desfire_record_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --write-offset 0 `
  --write-hex "01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10" `
  --commit
```
