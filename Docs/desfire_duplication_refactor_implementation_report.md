# DESFire Duplication Refactor - Implementation Report

## Scope
This document records the concrete refactor work implemented from `Docs/desfire_commands_duplication_report.md`.

## Design Constraints Followed
- Kept the existing command-class structure and command interfaces.
- Preserved ETL-first style (`etl::vector`, `etl::array`, `etl::expected`) and avoided introducing dynamic-allocation-only abstractions.
- Kept secure-messaging behavior inside command-layer helpers (no change to wire/APDU adapter layers).
- Applied low-risk incremental changes: shared helper extraction + command call-site migration.

## Implemented Refactors

### 1) Shared AES plain authenticated command helpers
Added and used shared helpers in:
- `Include/Nfc/Desfire/Commands/ValueOperationCryptoUtils.h`

Added helpers:
- `appendLe24(...)`
- `deriveAesPlainRequestIv(...)`
- `deriveAesPlainResponseIv(...)`
- `verifyAesAuthenticatedPlainPayload(...)`
- `setContextIv(...)`

These now centralize CMAC-based IV progression and authenticated plain-response verification.

### 2) Clear/Delete command CMAC/IV duplication removed
Migrated to shared helpers:
- `Src/Nfc/Desfire/Commands/ClearRecordFileCommand.cpp`
- `Src/Nfc/Desfire/Commands/DeleteFileCommand.cpp`

Removed command-local duplicated request/response IV helper logic and now use shared request/response IV APIs.

### 3) Commit/Format command-only IV pattern unified
Migrated to shared helpers:
- `Src/Nfc/Desfire/Commands/CommitTransactionCommand.cpp`
- `Src/Nfc/Desfire/Commands/FormatPiccCommand.cpp`

Both commands now use the same command-byte CMAC request-IV derivation and shared response-IV progression.

### 4) FreeMemory and GetFileSettings authenticated plain handling unified
Migrated to shared helpers:
- `Src/Nfc/Desfire/Commands/FreeMemoryCommand.cpp`
- `Src/Nfc/Desfire/Commands/GetFileSettingsCommand.cpp`

Header cleanup:
- `Include/Nfc/Desfire/Commands/FreeMemoryCommand.h`
- `Include/Nfc/Desfire/Commands/GetFileSettingsCommand.h`

Removed duplicated request-IV methods and switched parse-time validation to shared payload verification.

### 5) Session cipher + CRC helper reuse extended
Refactored command-local CRC/session logic to rely on shared utility mapping/delegation in:
- `Src/Nfc/Desfire/Commands/GetValueCommand.cpp`
- `Src/Nfc/Desfire/Commands/GetCardUidCommand.cpp`
- `Src/Nfc/Desfire/Commands/SetConfigurationCommand.cpp`
- `Src/Nfc/Desfire/Commands/ChangeKeySettingsCommand.cpp`
- `Src/Nfc/Desfire/Commands/ChangeFileSettingsCommand.cpp`

Result: reduced algorithm duplication and aligned cipher/CRC behavior to one utility source.

### 6) Removed local AES-CMAC implementation duplication from GetValue
- `Src/Nfc/Desfire/Commands/GetValueCommand.cpp`

Replaced internal CMAC primitive implementation with calls to shared `valueop_detail` helper APIs.

### 7) Value mutation triplet consolidated
Added shared helper:
- `Include/Nfc/Desfire/Commands/ValueMutationCommandUtils.h`

Migrated commands:
- `Src/Nfc/Desfire/Commands/CreditCommand.cpp`
- `Src/Nfc/Desfire/Commands/DebitCommand.cpp`
- `Src/Nfc/Desfire/Commands/LimitedCreditCommand.cpp`

Header cleanup:
- `Include/Nfc/Desfire/Commands/CreditCommand.h`
- `Include/Nfc/Desfire/Commands/DebitCommand.h`
- `Include/Nfc/Desfire/Commands/LimitedCreditCommand.h`

Result: one shared implementation path for validation, request building, and response/IV handling.

### 8) Create-file pair duplication consolidated
Added shared helper:
- `Include/Nfc/Desfire/Commands/CreateFileCommandUtils.h`

Migrated commands:
- `Src/Nfc/Desfire/Commands/CreateStdDataFileCommand.cpp`
- `Src/Nfc/Desfire/Commands/CreateBackupDataFileCommand.cpp`
- `Src/Nfc/Desfire/Commands/CreateLinearRecordFileCommand.cpp`
- `Src/Nfc/Desfire/Commands/CreateCyclicRecordFileCommand.cpp`

Result: shared option validation, access-right packing, common request framing, and simple parse handling.

## Build Verification
Refactor was validated with a full debug build:
- Command run: `cmake --build build --config Debug --parallel 8`
- Result: **success** (library, tests, and examples compiled).

## Notes
- This implementation focuses on the highest-value duplication items from the report (crypto/session helpers and near-identical command families).
- Existing command APIs and example CLIs were preserved.
