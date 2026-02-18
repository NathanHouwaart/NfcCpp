# DESFire ChangeFileSettings Command Guide

## Purpose

`ChangeFileSettings` updates communication mode and access rights of one file in the selected application.

- Command code: `0x5F`
- Scope: selected application + target file number

Use this command when you need to:

1. Change file communication mode (`plain`, `MAC`, `enciphered`).
2. Change access-right nibbles for read, write, read+write, and change-rights.

## Payload and Response

Logical payload fields:

1. `FileNo` (1 byte, `0..31`)
2. `CommSettings` (1 byte: `0x00`, `0x01`, `0x03`)
3. Access byte 1: `(ReadWriteAccess << 4) | ChangeAccess`
4. Access byte 2: `(ReadAccess << 4) | WriteAccess`

Response:

- status code (`0x00` on success)
- optional trailing MAC/CMAC bytes in authenticated sessions

## Access-Right Nibbles

Each access nibble is `0x0..0xF`:

- `0x0..0xD`: specific key number (`key0..key13`)
- `0x0E`: free access
- `0x0F`: never

## Command-Protection Mode

In this repository implementation you can control how `ChangeFileSettings` itself is sent:

- `0x00` plain
- `0x01` MAC (currently not implemented)
- `0x03` enciphered
- `0xFF` auto (`enc` when authenticated, otherwise `plain`)

## Preconditions

Before calling `ChangeFileSettings`:

1. Select the correct application AID.
2. Ensure `fileNo` exists.
3. Authenticate if file policy requires it.
4. Validate that new access-right values are intentional.

Common errors:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`
- `ParameterError (0x9E)`
- `IntegrityError (0x1E)` when secure-session framing/IV state is inconsistent

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/ChangeFileSettingsCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/ChangeFileSettingsCommand.cpp`
- Card API:
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp` (`DesfireCard::changeFileSettings`)
- Example:
  - `examples/desfire_change_file_settings/main.cpp`

## Example

```powershell
.\build\examples\Debug\desfire_change_file_settings_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --new-comm-mode enc `
  --read-access key0 `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --confirm-change
```

