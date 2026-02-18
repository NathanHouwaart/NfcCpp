# DESFire GetKeyVersion Command Guide

## Purpose

`GetKeyVersion` returns the version byte of a specific key number in the currently selected PICC/application.

- Command code: `0x64`
- Request payload: key number (1 byte)
- Response payload: key version (1 byte)

## Request

- Select target AID first (`SelectApplication`)
- Optionally authenticate depending on key settings
- Call `GetKeyVersion(keyNo)`

## Implementation

- Header: `Include/Nfc/Desfire/Commands/GetKeyVersionCommand.h`
- Source: `Src/Nfc/Desfire/Commands/GetKeyVersionCommand.cpp`
- Card API:
  - `DesfireCard::getKeyVersion(uint8_t keyNo)` in `Include/Nfc/Desfire/DesfireCard.h`
  - `DesfireCard::getKeyVersion(uint8_t keyNo)` in `Src/Nfc/Desfire/DesfireCard.cpp`

## Notes

- In authenticated sessions additional trailing integrity bytes may be present.  
  The parser reads the version byte from the start of payload.
- For DES-family keys, version bits are related to key parity/version handling conventions.

