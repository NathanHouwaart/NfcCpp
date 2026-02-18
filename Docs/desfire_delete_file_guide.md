# DESFire DeleteFile Command Guide

## Purpose

`DeleteFile` removes one file from the currently selected DESFire application.

- Command code: `0xDF`
- Target: one file number (`fileNo`)

Use this when you want to remove data/value/record files you no longer need.

## Payload and Response

Command payload:

1. `FileNo` (1 byte, `0..31`)

Response:

- status code (`0x00` on success)
- optional trailing MAC/CMAC bytes in authenticated sessions

## Command Options and Practical Options

At command level, `DeleteFile` has one parameter:

- `fileNo`

Practical execution options around the command:

1. Selected application (`SelectApplication` must point to the file owner app).
2. Authentication state (required depending on file/application access rights).
3. Optional handling for missing file (`FileNotFound (0xF0)`).

## Preconditions

Before deleting a file:

1. Select the correct application.
2. Authenticate if required by access rights.
3. Ensure `fileNo` is in range (`0..31`).

Common errors:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`
- `FileNotFound (0xF0)`

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/DeleteFileCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/DeleteFileCommand.cpp`
- Card helper:
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp` (`DesfireCard::deleteFile`)
- Example:
  - `examples/desfire_delete_file/main.cpp`

## Example

```powershell
.\build\examples\Debug\desfire_delete_file_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5
```

