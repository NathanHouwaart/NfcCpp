# DESFire FreeMemory Command Guide

## Purpose

`FreeMemory` queries the remaining free memory on the DESFire PICC.

- Command code: `0x6E`
- Scope: PICC-level (typically with selected AID `000000`)

Use this command before provisioning applications/files to estimate available capacity.

## Payload and Response

Command payload:

- none

Response payload:

1. `FreeMemory` (3 bytes, little-endian)

This 24-bit value is the available free memory in bytes.

## Command Options and Practical Options

At command level, `FreeMemory` has no payload parameters.

Practical options around execution:

1. Select PICC (`SelectApplication(000000)`) before the command.
2. Authenticate at PICC level when card permissions require it.
3. Interpret result in bytes/KiB for planning capacity.

## Preconditions

Before calling `FreeMemory`:

1. Ensure a card is selected.
2. Select AID `000000` (PICC context).
3. Authenticate if PICC key settings require it.

Common errors:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`
- `InvalidResponse` if transport/security framing is malformed

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/FreeMemoryCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/FreeMemoryCommand.cpp`
- Card helper:
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp` (`DesfireCard::freeMemory`)
- Example:
  - `examples/desfire_free_memory/main.cpp`

## Example

```powershell
.\build\examples\Debug\desfire_free_memory_example.exe COM5 `
  --aid 000000 `
  --authenticate `
  --auth-mode iso  `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
```

