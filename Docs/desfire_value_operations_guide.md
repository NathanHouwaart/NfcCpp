# DESFire Value Operations Command Guide

This guide covers these commands:

- `GetValue` (`INS 0x6C`)
- `Credit` (`INS 0x0C`)
- `Debit` (`INS 0xDC`)
- `LimitedCredit` (`INS 0x1C`)

These commands operate on DESFire value files created with `CreateValueFile`.

## Command Payloads

### GetValue (`0x6C`)

Payload:

1. `FileNo` (1 byte)

Response payload:

- 4-byte signed value (little-endian), potentially followed by secure messaging bytes depending on session mode.

### Credit (`0x0C`)

Payload:

1. `FileNo` (1 byte)
2. `Value` (4 bytes, little-endian signed integer)

### Debit (`0xDC`)

Payload:

1. `FileNo` (1 byte)
2. `Value` (4 bytes, little-endian signed integer)

### LimitedCredit (`0x1C`)

Payload:

1. `FileNo` (1 byte)
2. `Value` (4 bytes, little-endian signed integer)

## Validation in This Implementation

- `fileNo` must be in `0..31`.
- `credit/debit/limited-credit` values must be non-negative.
- `GetValue` parser expects a 4-byte value payload, with tolerance for common trailing secure-messaging bytes.

Invalid input returns `ParameterError` before sending.

## Transaction Note

For many DESFire configurations, `Credit`, `Debit`, and `LimitedCredit` are transactional and require `CommitTransaction` to persist.

If no commit is performed, changes may be discarded depending on card behavior/session lifecycle.

The `desfire_value_operations_example` supports `--commit` to commit in the same run.

See also: `Docs/desfire_commit_transaction_guide.md`.

## Code Locations

- `GetValue`
  - `Include/Nfc/Desfire/Commands/GetValueCommand.h`
  - `Src/Nfc/Desfire/Commands/GetValueCommand.cpp`
- `Credit`
  - `Include/Nfc/Desfire/Commands/CreditCommand.h`
  - `Src/Nfc/Desfire/Commands/CreditCommand.cpp`
- `Debit`
  - `Include/Nfc/Desfire/Commands/DebitCommand.h`
  - `Src/Nfc/Desfire/Commands/DebitCommand.cpp`
- `LimitedCredit`
  - `Include/Nfc/Desfire/Commands/LimitedCreditCommand.h`
  - `Src/Nfc/Desfire/Commands/LimitedCreditCommand.cpp`
- `DesfireCard` wrappers
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`

## Example

Use:

`examples/desfire_value_operations/main.cpp`

Run:

```powershell
.\build\examples\Debug\desfire_value_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 3 `
  --op credit `
  --value 25 `
  --show-before `
  --show-after
```
