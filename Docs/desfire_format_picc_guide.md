# DESFire FormatPICC Command Guide

## Purpose

`FormatPICC` erases the complete DESFire PICC content.

- Command code: `0xFC`
- Scope: PICC-level

After formatting, applications/files are removed and the card is returned to empty PICC state.

## Payload and Response

Command payload:

- none

Response:

- status code (`0x00` on success)
- optional trailing MAC/CMAC bytes in authenticated sessions

## Command Options and Practical Options

At command level, `FormatPICC` has no payload parameters.

Practical options around execution:

1. Select PICC (`SelectApplication(000000)`) before running it.
2. Authenticate PICC master key when required by key settings.
3. Use an explicit confirmation step in tooling/scripts because command is destructive.

## Preconditions

Before calling `FormatPICC`:

1. Ensure the correct card is present.
2. Select AID `000000` (PICC context).
3. Authenticate PICC master key if required.

Common errors:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`
- `IntegrityError` when secure-session framing/IV state is inconsistent

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/FormatPiccCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/FormatPiccCommand.cpp`
- Card helper:
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp` (`DesfireCard::formatPicc`)
- Example:
  - `examples/desfire_format_picc/main.cpp`

## Example

```powershell
.\build\examples\Debug\desfire_format_picc_example.exe COM5 `
  --aid 000000 `
  --authenticate `
  --auth-mode iso `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" `
  --confirm
```

