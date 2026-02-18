# DESFire SetConfiguration Command Guide

## Purpose

`SetConfiguration` updates PICC-level configuration on a DESFire card.

- Command code: `0x5C`
- Scope: PICC level (select AID `000000` and authenticate first)

This command is different from key policy commands and is used for behavior/configuration fields such as random UID and ATS.

## Supported Subcommands in This Repository

### 1. PICC configuration byte (`subcommand 0x00`)

Payload: one byte.

Implemented/commonly used bits:

- Bit `0` (`0x01`): disable format
- Bit `1` (`0x02`): enable random UID mode

You send the full byte; helper options in the example can build it from individual bits.

### 2. ATS configuration (`subcommand 0x01`)

Payload: ATS bytes.

- ATS typically includes TL as byte 0.
- The example accepts ATS with or without TL and normalizes it.
- Maximum ATS length including TL is validated to 20 bytes.

## Security/Framing Behavior

- Command implementation:
  - `Include/Nfc/Desfire/Commands/SetConfigurationCommand.h`
  - `Src/Nfc/Desfire/Commands/SetConfigurationCommand.cpp`
- Card API wrappers:
  - `DesfireCard::setConfigurationPicc(...)`
  - `DesfireCard::setConfigurationAts(...)`

Encryption behavior follows the same secure-command pattern used by existing encrypted DESFire commands in this codebase:

- CRC over command bytes + plaintext payload
- CBC encryption with current session context
- Legacy DES send-mode handling when applicable
- IV update for CBC modes that require it

## How This Differs from ChangeKeySettings

`ChangeKeySettings` (`0x54`) and `SetConfiguration` (`0x5C`) are often confused:

- `ChangeKeySettings` changes access policy bits in KeySettings1:
  who can change keys, list, create/delete files/apps, etc.
- `SetConfiguration` changes PICC behavior/configuration:
  random UID, format behavior, ATS configuration.

Use:

- `ChangeKeySettings` for access-control policy.
- `SetConfiguration` for PICC operating behavior.

## Example

See:

- `examples/desfire_set_configuration/main.cpp`
- `examples/desfire_set_configuration/README.md`

## Safety Notes

- Some SetConfiguration changes can be difficult or impossible to revert depending on card/platform behavior.
- Always validate with a dry run first and keep known-good credentials and test cards during development.
