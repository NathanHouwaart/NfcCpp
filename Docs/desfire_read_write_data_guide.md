# DESFire ReadData + WriteData Command Guide

## Purpose

These commands move payload bytes between host and DESFire standard/backup data files.

- `ReadData` command code: `0xBD`
- `WriteData` command code: `0x3D`

Both commands are application-level operations and typically require:

1. `SelectApplication(targetAID)`
2. `Authenticate` with a key that has matching file access rights

## Payload Layout

### ReadData (`0xBD`)

Request payload:

1. `FileNo` (1 byte)
2. `Offset` (3 bytes, little-endian)
3. `Length` (3 bytes, little-endian)

Response payload:

- Requested file bytes (possibly split over additional-frame responses)

### WriteData (`0x3D`)

Request payload:

1. `FileNo` (1 byte)
2. `Offset` (3 bytes, little-endian)
3. `Length` (3 bytes, little-endian)
4. `Data` (`Length` bytes)

Response payload:

- Usually empty on success (`0x00` status)

## This Implementation

### ReadData

- Class: `ReadDataCommand`
- Files:
  - `Include/Nfc/Desfire/Commands/ReadDataCommand.h`
  - `Src/Nfc/Desfire/Commands/ReadDataCommand.cpp`
- Behavior:
  - Chunked reads across command cycles
  - Supports `0xAF` continuation frames inside a chunk
  - Trims trailing authenticated response MAC/CMAC bytes when present

### WriteData

- Class: `WriteDataCommand`
- Files:
  - `Include/Nfc/Desfire/Commands/WriteDataCommand.h`
  - `Src/Nfc/Desfire/Commands/WriteDataCommand.cpp`
- Behavior:
  - Chunked writes across command cycles
  - Sends each chunk as a complete `WriteData` command

### Card API

Exposed on `DesfireCard`:

- `readData(fileNo, offset, length, chunkSize)`
- `writeData(fileNo, offset, data, chunkSize)`

Files:

- `Include/Nfc/Desfire/DesfireCard.h`
- `Src/Nfc/Desfire/DesfireCard.cpp`

## Parameter Validation

Implemented checks include:

- `fileNo` in `0..31`
- `offset` within 24-bit range
- length within 24-bit range
- `(offset + length)` must stay within DESFire 24-bit addressable space
- chunk size capped at `240`

Read-side implementation limit:

- Maximum returned data buffer: `4096` bytes (`DesfireCard::MAX_DATA_IO_SIZE`)

## Common Errors

- `PermissionDenied (0x9D)`: key has no read/write rights
- `AuthenticationError (0xAE)`: auth missing/wrong key
- `LengthError (0x7E)`: invalid offset/length for file boundaries
- `FileNotFound (0xF0)`: wrong file number in selected app

## Example

Use:

- `examples/desfire_read_write_data/main.cpp`
- `examples/desfire_read_write_data/README.md`

