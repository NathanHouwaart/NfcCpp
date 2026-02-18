# DESFire CreateValueFile Command Guide

## Purpose

`CreateValueFile` creates a value file in the currently selected application.

- Command code: `0xCC`
- Security: application-level permission (typically requires app master key auth)

Value files are used for signed 32-bit counters/balances manipulated via `Credit`, `Debit`, `LimitedCredit`, and `GetValue`.

## Payload Layout

`CreateValueFile` uses a 17-byte payload:

1. `FileNo` (1 byte)
2. `CommunicationSettings` (1 byte)
3. `AccessRights` (2 bytes)
4. `LowerLimit` (4 bytes, little-endian, signed)
5. `UpperLimit` (4 bytes, little-endian, signed)
6. `LimitedCreditValue` (4 bytes, little-endian, signed)
7. `ValueOptions` (1 byte)

### CommunicationSettings

- `0x00`: plain
- `0x01`: plain + MAC
- `0x03`: enciphered

### AccessRights Encoding

Access rights are sent as two bytes:

- Byte0 = `[RW|CAR]`
- Byte1 = `[R|W]`

Nibble values:

- `0x0..0xD`: key number 0..13
- `0x0E`: free access
- `0x0F`: never

### ValueOptions Bits

- bit0 (`0x01`): limited credit enabled
- bit1 (`0x02`): free get value enabled

Bits outside `0x03` are reserved and rejected by this implementation.

## Preconditions

Before creating the file:

1. `SelectApplication(target AID)`
2. `Authenticate` with a key allowed to create files (often app master key)

If these are not met, common responses are:

- `PermissionDenied (0x9D)`
- `AuthenticationError (0xAE)`

## Validation in This Implementation

The command validates:

- `fileNo` in range `0..31`
- `communicationSettings` is one of `0x00`, `0x01`, `0x03`
- access-right nibbles in range `0x0..0xF`
- `lowerLimit <= upperLimit`
- `limitedCreditValue` inside `[lowerLimit, upperLimit]`
- `valueOptions` uses only bits `0..1`

Invalid inputs return `ParameterError`.

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/CreateValueFileCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/CreateValueFileCommand.cpp`
- Card helper method: `DesfireCard::createValueFile(...)` in
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

## What This Command Does

`CreateValueFile` defines a value-file object and its configuration:

- value boundaries,
- limited-credit settings,
- communication mode,
- key-based access control.

It does not perform balance updates by itself. Use `Credit`, `Debit`, `LimitedCredit`, and `GetValue` after creation.

## Relation To ChangeKeySettings

`ChangeKeySettings` and `CreateValueFile` are complementary:

- `ChangeKeySettings` sets app-level security policy (listing/create/delete and key-change governance).
- `CreateValueFile` creates one value file with its own access rights and bounds.

Use `ChangeKeySettings` for policy, `CreateValueFile` for file structure and behavior.

## Example

Use:

`examples/desfire_create_value_file/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_create_value_file_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 3 `
  --comm-mode enc `
  --read-access key0 `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --lower-limit -1000 `
  --upper-limit 1000 `
  --limited-credit-value 100 `
  --limited-credit-enabled
```

