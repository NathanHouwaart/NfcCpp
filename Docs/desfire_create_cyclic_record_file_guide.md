# DESFire CreateCyclicRecordFile Command Guide

## Purpose

`CreateCyclicRecordFile` creates a cyclic record file in the currently selected application.

- Command key (INS): `0xC0`
- Security: application-level permission (typically requires app master key auth)

A cyclic record file stores fixed-size records in a ring buffer. When full, new writes overwrite the oldest records.

## Payload Layout

`CreateCyclicRecordFile` uses a 10-byte payload:

1. `FileNo` (1 byte)
2. `CommunicationSettings` (1 byte)
3. `AccessRights` (2 bytes)
4. `RecordSize` (3 bytes, LSB first)
5. `MaxRecords` (3 bytes, LSB first)

### CommunicationSettings

- `0x00`: plain
- `0x01`: plain + MAC
- `0x03`: enciphered

### AccessRights Encoding

Access rights are sent as two bytes:

- Byte0 = `[RW|CAR]`
- Byte1 = `[R|W]`

Nibble values:

- `0x0..0xD`: key number 0..13
- `0x0E`: free access
- `0x0F`: never

## Preconditions

Before creating the file:

1. `SelectApplication(target AID)`
2. `Authenticate` with a key allowed to create files (often app master key)

If these are not met, common responses are:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`

## Cyclic Record Behavior

Cyclic record files are rolling buffers:

- `WriteRecord` appends records.
- Once full, appending a record overwrites the oldest stored record.
- `ReadRecords` returns records by logical order.
- `ClearRecordFile` removes all records.

Use cyclic files for bounded history (for example recent events), where fixed storage is more important than retaining all historical entries.

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/CreateCyclicRecordFileCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/CreateCyclicRecordFileCommand.cpp`
- Card helper method: `DesfireCard::createCyclicRecordFile(...)` in
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

## Validation in This Implementation

The command validates:

- `fileNo` in range `0..31`
- `communicationSettings` is one of `0x00`, `0x01`, `0x03`
- access right nibbles in range `0x0..0xF`
- `recordSize` in range `1..0xFFFFFF`
- `maxRecords` in range `1..0xFFFFFF`

Invalid inputs return `ParameterError`.

## Relation To ChangeKeySettings

`ChangeKeySettings` and `CreateCyclicRecordFile` are different:

- `ChangeKeySettings` changes key-policy bits for the selected PICC/application.
- `CreateCyclicRecordFile` creates a new cyclic record file with file-level access rights.

Use `ChangeKeySettings` for policy, `CreateCyclicRecordFile` for storage layout.

## Example

Use:

`examples/desfire_create_cyclic_record_file/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_create_cyclic_record_file_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 6 `
  --comm-mode enc `
  --read-access key0 `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --record-size 16 `
  --max-records 20
```

