# DESFire CreateBackupDataFile Command Guide

## Purpose

`CreateBackupDataFile` creates a backup data file in the currently selected application.

- Command code: `0xCB`
- Security: application-level permission (typically requires app master key auth)

## Payload Layout

`CreateBackupDataFile` uses a 7-byte payload:

1. `FileNo` (1 byte)
2. `CommunicationSettings` (1 byte)
3. `AccessRights` (2 bytes)
4. `FileSize` (3 bytes, LSB first)

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

## Backup File Behavior

Backup data files are transactional:

- `WriteData` modifies a transaction buffer.
- `CommitTransaction` makes the update persistent.
- `AbortTransaction` discards pending updates.

This command only creates the backup file object and its metadata.

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/CreateBackupDataFileCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/CreateBackupDataFileCommand.cpp`
- Card helper method: `DesfireCard::createBackupDataFile(...)` in
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

## Validation in This Implementation

The command validates:

- `fileNo` in range `0..31`
- `communicationSettings` is one of `0x00`, `0x01`, `0x03`
- access right nibbles in range `0x0..0xF`
- `fileSize` in range `1..0xFFFFFF`

Invalid inputs return `ParameterError`.

## Relation To ChangeKeySettings

`ChangeKeySettings` and `CreateBackupDataFile` serve different purposes:

- `ChangeKeySettings` updates `KeySettings1` policy bits (who can change keys/list/create/delete).
- `CreateBackupDataFile` creates a file container with access rights and size.

Use `ChangeKeySettings` for security policy, and `CreateBackupDataFile` for storage layout.

## Example

Use:

`examples/desfire_create_backup_data_file/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_create_backup_data_file_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 1 `
  --comm-mode plain `
  --read-access key0 `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --file-size 32
```


