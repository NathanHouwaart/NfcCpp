# DESFire ChangeKeySettings Command Guide

## Purpose

`ChangeKeySettings` updates the **KeySettings1** byte of the currently selected PICC/application.

- Command code: `0x54`
- Scope: selected AID (`000000` for PICC, or an application AID)
- Security: encrypted command payload; requires an authenticated session

Use this command to define:

- whether master key/config can still be changed,
- whether certain PICC-level operations can run without PICC master auth,
- which key is required for future key changes.

## KeySettings1 Layout

`KeySettings1` is one byte:

- Bit 0 (`0x01`): allow master key change
- Bit 1 (`0x02`): allow `GetApplicationIDs` / `GetKeySettings` without PICC master auth
- Bit 2 (`0x04`): allow `CreateApplication` / `DeleteApplication` without PICC master auth
- Bit 3 (`0x08`): configuration/key-settings changeable
- Bits 4..7 (`0xF0`): key-change access rule

Key-change access rule (upper nibble):

- `0x0`: master key required
- `0x1..0xD`: key1..key13 required
- `0xE`: authenticate with the same key being changed
- `0xF`: all key changes frozen

## Typical Values

- `0x0F`: common factory-default style value
- `0x00`: most restrictive low bits, MK required for key changes
- `0xF0`: key changes frozen (dangerous unless intentional)

## Implementation in This Repository

- Header: `Include/Nfc/Desfire/Commands/ChangeKeySettingsCommand.h`
- Source: `Src/Nfc/Desfire/Commands/ChangeKeySettingsCommand.cpp`
- Card API:
  - `DesfireCard::changeKeySettings(...)` in `Include/Nfc/Desfire/DesfireCard.h`
  - `DesfireCard::changeKeySettings(...)` in `Src/Nfc/Desfire/DesfireCard.cpp`
- Example:
  - `examples/desfire_change_key_settings/main.cpp`
  - `examples/desfire_change_key_settings/README.md`

## Notes on Session Cipher Selection

`ChangeKeySettingsCommand` supports:

- `legacy` + DES/2K3DES send-mode handling,
- ISO/AES CBC handling,
- optional explicit session-key-type override (`DES`, `2K3DES`, `3K3DES`, `AES`) for ambiguous 16-byte sessions.

## Safety Recommendations

- Validate authentication first before applying settings.
- Keep a rollback plan (known key + known settings).
- Avoid freezing key changes (`0xF?`) unless you are certain this is final.
