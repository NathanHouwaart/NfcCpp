# DESFire DeleteApplication Guide

This guide explains how `DeleteApplication` is implemented in this repository and how to use it safely.

## Command Summary

- DESFire command: `DeleteApplication`
- INS: `0xDA`
- Payload: 3 bytes
  - `AID[0] AID[1] AID[2]`

In this repo the command is implemented as:

- `Include/Nfc/Desfire/Commands/DeleteApplicationCommand.h`
- `Src/Nfc/Desfire/Commands/DeleteApplicationCommand.cpp`

and exposed via:

- `DesfireCard::deleteApplication(...)`

## Preconditions

Before calling `DeleteApplication`, you should:

1. Select PICC application (`AID = 00 00 00`)
2. Authenticate with PICC master key (unless PICC settings allow create/delete without PICC MK auth)

If these are not met, typical card responses are:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`

## Typical Flow

1. `SelectApplication(000000)`
2. `Authenticate(PICC master key)`
3. `DeleteApplication(target AID)`

## Expected Responses

- `Ok (0x00)`: application deleted
- `ApplicationNotFound (0xA0)`: target AID does not exist
- `PermissionDenied (0x9D)`: missing required PICC permissions/authentication
- `AuthenticationError (0xAE)`: wrong auth mode/key

## Implementation Notes

- Command is single-frame (no `0xAF` chaining).
- Request data is built as AID bytes in DESFire APDU order.
- No dynamic allocation is used in command implementation:
  - ETL containers only
  - state machine style: `buildRequest`, `parseResponse`, `isComplete`, `reset`

