# DESFire GetKeySettings Command Guide

## Purpose

`GetKeySettings` retrieves the key settings bytes of the currently selected PICC/application.

- Command code: `0x45`
- Scope: currently selected AID
- Response: key settings bytes

## Response Bytes

The command exposes two primary bytes:

- `KeySettings1`: access policy bits for master key settings
- `KeySettings2`: key count + key type flags

`KeySettings2` layout:

- low nibble (`bits 0..3`): key count
- key type flags (`bits 6..7`):
  - `0x00`: DES / 2K3DES family
  - `0x40`: 3K3DES
  - `0x80`: AES

## Implementation

- Header: `Include/Nfc/Desfire/Commands/GetKeySettingsCommand.h`
- Source: `Src/Nfc/Desfire/Commands/GetKeySettingsCommand.cpp`
- Card API:
  - `DesfireCard::getKeySettings()` in `Include/Nfc/Desfire/DesfireCard.h`
  - `DesfireCard::getKeySettings()` in `Src/Nfc/Desfire/DesfireCard.cpp`

## Notes

- Some configurations return additional trailing integrity bytes in authenticated sessions.  
  The parser reads the required key-setting bytes from the start of payload.
- Permission requirements depend on card/application key settings.

