# DESFire GetCardUID Command Guide

## Purpose

`GetCardUID` reads the real card UID (7 bytes).

- Command code: `0x51`
- Scope: PICC-level
- Typical requirement: authenticate with PICC master key first

## Request/Response

Request:

- `INS=0x51`
- no payload

Response:

- On success, UID is returned
- In authenticated sessions, response can be encrypted with integrity bytes

## Implementation Notes

The command implementation:

- Parses status and payload from `PDU = [Status][Data...]`
- In authenticated sessions, tries encrypted decode and CRC verification:
  - DES / 2K3DES: CRC16 over UID
  - AES / 3K3DES: DESFire CRC32 over `UID + 0x00`
- Falls back to first 7 bytes when payload appears plain/tailed

Code:

- Header: `Include/Nfc/Desfire/Commands/GetCardUidCommand.h`
- Source: `Src/Nfc/Desfire/Commands/GetCardUidCommand.cpp`
- Card API: `DesfireCard::getRealCardUid()` in `Src/Nfc/Desfire/DesfireCard.cpp`

## Example

Use:

`examples/desfire_get_card_uid/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_get_card_uid_example.exe COM5 `
  --auth-mode legacy `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00"
```

