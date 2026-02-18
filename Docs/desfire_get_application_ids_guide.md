# DESFire GetApplicationIDs Command Guide

## Purpose

`GetApplicationIDs` retrieves all application identifiers present on the card.

- Command code: `0x6A`
- Continuation code: `0xAF`
- Security: depends on PICC key settings (may require PICC authentication)

## APDU/Frame Behavior

`GetApplicationIDs` can be single-frame or multi-frame:

1. Send `0x6A`
2. If status is `0xAF`, send `0xAF` and continue
3. Stop when status is `0x00`

All payload bytes are concatenated and then parsed into 3-byte AID entries.

## Payload Parsing

- Raw payload is a sequence of AID triplets.
- Final payload length must be divisible by 3.
- Parsed output is exposed as an ETL vector of `etl::array<uint8_t, 3>`.

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/GetApplicationIdsCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/GetApplicationIdsCommand.cpp`
- Card helper method: `DesfireCard::getApplicationIds()` in `Include/Nfc/Desfire/DesfireCard.h` and `Src/Nfc/Desfire/DesfireCard.cpp`

## When Authentication Is Needed

If `GetApplicationIDs` returns permission/authentication errors, use PICC authentication first:

1. `SelectApplication(000000)`
2. `Authenticate(PICC key)`
3. `GetApplicationIDs`

## Example

Use:

`examples/desfire_get_application_ids/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_get_application_ids_example.exe COM5 `
  --authenticate `
  --auth-mode iso `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
```

