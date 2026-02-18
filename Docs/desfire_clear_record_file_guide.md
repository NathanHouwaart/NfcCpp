# DESFire ClearRecordFile Command Guide

## Purpose

`ClearRecordFile` resets a linear or cyclic record file to an empty state in the currently selected application.

- Command code: `0xEB`
- Target: one record file number (`fileNo`)

This is used when you want to clear all existing records without deleting the file itself.

## Payload and Response

Command payload:

1. `FileNo` (1 byte, `0..31`)

Response:

- status code (`0x00` on success)
- optional trailing MAC/CMAC bytes in authenticated sessions

## Command Options and Practical Options

At command level, `ClearRecordFile` has one parameter:

- `fileNo`

Practical execution options around the command:

1. Selected application (`SelectApplication` must point to the file owner app).
2. Authentication state (required depending on file/application access rights).
3. Transaction handling (`CommitTransaction` to persist clear).

## Transaction Behavior

`ClearRecordFile` is transactional on DESFire.

After successful clear command acceptance, call:

- `CommitTransaction (0xC7)`

Without commit, pending clear changes may be discarded.

## Preconditions

Before clearing records:

1. Select the correct application.
2. Authenticate if required by access rights.
3. Ensure `fileNo` points to a linear/cyclic record file.

Common errors:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`
- `FileNotFound (0xF0)`
- `BoundaryError (0xBE)` in invalid file-state/range scenarios

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/ClearRecordFileCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/ClearRecordFileCommand.cpp`
- Card helper:
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp` (`DesfireCard::clearRecordFile`)
- Example:
  - `examples/desfire_clear_record_file/main.cpp`

## Example

```powershell
.\build\examples\Debug\desfire_clear_record_file_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --commit
```

