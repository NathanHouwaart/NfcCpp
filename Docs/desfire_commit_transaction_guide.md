# DESFire CommitTransaction Command Guide

## Purpose

`CommitTransaction` finalizes pending transactional updates in the currently selected DESFire application.

- Command code: `0xC7`
- Typical use: after transactional writes/updates

Examples:

- backup data file writes (`WriteData` to backup file)
- value file updates (`Credit`, `Debit`, `LimitedCredit`)

Without commit, many cards keep changes in a transaction buffer and do not persist them.

## Payload and Response

Command payload:

- none (empty payload)

Response:

- status code (`0x00` on success)
- in authenticated sessions, some cards may append MAC/CMAC bytes

## "Command Options" and Practical Options

At command level, `CommitTransaction (0xC7)` has no payload options in this implementation.

Practical operation options are around execution flow:

1. Which application is selected before commit (`SelectApplication`).
2. Whether authentication is required by access rules.
3. How to handle `NoChanges (0x0C)` if no pending transaction exists.

In the example tool, `--allow-no-changes` maps #3 to a non-fatal path.

## Preconditions

Before calling commit:

1. Select the correct application.
2. Perform any transactional operation that created pending changes.
3. Authenticate if required by file/application access configuration.

Common status outcomes:

- `0x00` (`Ok`): transaction committed.
- `0x0C` (`NoChanges`): no pending transaction found.
- `0x9D` (`PermissionDenied`) or `0xAE` (`AuthenticationError`): missing/insufficient auth.

## Code Locations

- Command header: `Include/Nfc/Desfire/Commands/CommitTransactionCommand.h`
- Command source: `Src/Nfc/Desfire/Commands/CommitTransactionCommand.cpp`
- Card helper method: `DesfireCard::commitTransaction()` in
  - `Include/Nfc/Desfire/DesfireCard.h`
  - `Src/Nfc/Desfire/DesfireCard.cpp`
- Example: `examples/desfire_commit_transaction/main.cpp`

## Example

```powershell
.\build\examples\Debug\desfire_commit_transaction_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

