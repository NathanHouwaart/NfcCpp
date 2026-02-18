# DESFire GetVersion Command Guide

## Purpose

`GetVersion` reads card version/manufacturing information from a DESFire PICC.

- Command code: `0x60`
- Continuation code: `0xAF`
- Security: plain mode (typically no authentication required)

## APDU/Frame Behavior

`GetVersion` is usually a multi-frame command:

1. Send `0x60`
2. Card returns payload chunk with status `0xAF` (additional frame)
3. Send `0xAF`
4. Repeat until final status `0x00`

The implementation in `GetVersionCommand` appends all payload bytes from each frame into one ETL buffer.

## Typical Payload Layout (EV1-style, 28 bytes)

- Bytes `0..6`: hardware block
- Bytes `7..13`: software block
- Bytes `14..20`: UID (7 bytes)
- Bytes `21..25`: batch number (5 bytes)
- Byte `26`: production week
- Byte `27`: production year

Cards/generations can differ. Always keep the raw payload available.

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/GetVersionCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/GetVersionCommand.cpp`
- Card helper method: `DesfireCard::getVersion()` in `Include/Nfc/Desfire/DesfireCard.h` and `Src/Nfc/Desfire/DesfireCard.cpp`

## Example

Use:

`examples/desfire_get_version/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_get_version_example.exe COM5
```

