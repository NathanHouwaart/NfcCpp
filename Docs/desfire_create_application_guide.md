# DESFire CreateApplication Guide

This guide explains how `CreateApplication` is implemented in this repository and how to use it safely.

## Command Summary

- DESFire command: `CreateApplication`
- INS: `0xCA`
- Payload (basic form): 5 bytes
  - `AID[0] AID[1] AID[2] KeySettings1 KeySettings2`

In this repo the command is implemented as:

- `Include/Nfc/Desfire/Commands/CreateApplicationCommand.h`
- `Src/Nfc/Desfire/Commands/CreateApplicationCommand.cpp`

and exposed via:

- `DesfireCard::createApplication(...)`

## Prerequisites

Before calling `CreateApplication`, you must:

1. Select PICC application (`AID = 00 00 00`)
2. Authenticate with PICC master key

If you skip these, the card usually returns `PermissionDenied (0x9D)` or `AuthenticationError (0xAE)`.

## KeySettings2 Encoding

`KeySettings2` is composed as:

- Lower nibble (`bits 0..3`): key count (`1..14`)
- Upper type bits:
  - `0x00` for `DES` / `2K3DES`
  - `0x40` for `3K3DES`
  - `0x80` for `AES`

Examples:

- 2 keys + AES: `0x82`
- 3 keys + 3K3DES: `0x43`
- 2 keys + 2K3DES: `0x02`

## KeySettings1

`KeySettings1` is passed as-is (application master key settings byte).

If you are unsure, use the commonly used default `0x0F` and adjust later with `ChangeKeySettings` once your flow is stable.

## Typical Flow

1. `SelectApplication(000000)` (PICC)
2. `Authenticate(PICC master key)`
3. `CreateApplication(target AID, keySettings1, keyCount, keyType)`
4. `SelectApplication(target AID)`
5. `Authenticate(application key 0)`

## Authentication Mode for New App

After creation, authenticate based on the app key type:

- `DES`: usually `legacy`
- `2K3DES`: use `iso`
- `3K3DES`: use `iso`
- `AES`: use `aes`

## Common Errors

- `DuplicateError (0xDE)`: application already exists
- `ParameterError (0x9E)`: invalid AID, key count, or settings
- `PermissionDenied (0x9D)`: not authenticated with PICC master key
- `AuthenticationError (0xAE)`: wrong auth mode or wrong key for current context

## No-Heap / ETL Notes

The implementation follows the same style as other DESFire commands:

- `etl::array` / `etl::vector` only
- no dynamic heap allocation
- state machine with `buildRequest` / `parseResponse` / `isComplete` / `reset`

