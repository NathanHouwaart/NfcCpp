# DESFire Commands Duplication Report

## Scope
- Folder analyzed: `Src/Nfc/Desfire/Commands`
- Files found: 34 `.cpp` files
- Commands compiled in `Src/Nfc/Desfire/CMakeLists.txt`: 33 command implementations
- Extra file not compiled as a command: `Src/Nfc/Desfire/Commands/changekey_command.cpp`

## Goal
Identify repeated functions / repeated code (especially secure-session CMAC / IV handling), then propose concrete, low-risk fixes.

## Summary
There is substantial duplication in:
- AES plain-command CMAC/IV progression
- session-cipher resolution and CRC helpers
- little-endian packing helpers
- boilerplate command request/response state machine
- near-identical command pairs that differ only by command code/name

This duplication increases maintenance cost and makes crypto behavior drift likely across commands.

## Findings

### 1. Value operation triplet is near-identical
Files:
- `Src/Nfc/Desfire/Commands/CreditCommand.cpp`
- `Src/Nfc/Desfire/Commands/DebitCommand.cpp`
- `Src/Nfc/Desfire/Commands/LimitedCreditCommand.cpp`

Observed:
- Same `buildRequest` flow and secure-messaging path
- Same response parsing and IV update behavior
- Same option validation structure
- Same `appendLe32` helper

Differences are mostly:
- command code (`0x0C`, `0xDC`, `0x1C`)
- command name string

Risk:
- Bug fix in one command can be missed in the other two.

---

### 2. `ClearRecordFile` and `DeleteFile` duplicate AES plain CMAC request/response helpers
Files:
- `Src/Nfc/Desfire/Commands/ClearRecordFileCommand.cpp`
- `Src/Nfc/Desfire/Commands/DeleteFileCommand.cpp`

Observed:
- Near-identical local helpers:
  - `deriveAesPlainRequestIv(...)`
  - `deriveAesPlainResponseIv(...)`
- Command flow differs mainly by INS (`0xEB` vs `0xDF`) and command name.

Risk:
- Any fix to CMAC verification logic must be duplicated manually.

---

### 3. `CommitTransaction` and `FormatPICC` duplicate the same `command-only request IV` pattern
Files:
- `Src/Nfc/Desfire/Commands/CommitTransactionCommand.cpp`
- `Src/Nfc/Desfire/Commands/FormatPiccCommand.cpp`

Observed:
- Local helper computes CMAC over one-byte message `[INS]`.
- Parse path updates AES IV via `valueop_detail::deriveAesResponseIvForValueOperation(...)`.
- Implementations are structurally identical except INS/name.

Risk:
- Future behavior changes to this pattern can diverge.

---

### 4. Create-file command pairs are duplicated templates
Files:
- Pair A:
  - `Src/Nfc/Desfire/Commands/CreateStdDataFileCommand.cpp`
  - `Src/Nfc/Desfire/Commands/CreateBackupDataFileCommand.cpp`
- Pair B:
  - `Src/Nfc/Desfire/Commands/CreateLinearRecordFileCommand.cpp`
  - `Src/Nfc/Desfire/Commands/CreateCyclicRecordFileCommand.cpp`

Observed:
- Same option validation, access-right packing, request format, parse/isComplete/reset.
- Differences are mostly INS and type/name.

Risk:
- Validation or framing fixes must be repeated in both commands of each pair.

---

### 5. Session cipher + CRC logic is re-implemented in multiple commands
Examples:
- `Src/Nfc/Desfire/Commands/ChangeKeyCommand.cpp`
- `Src/Nfc/Desfire/Commands/ChangeKeySettingsCommand.cpp`
- `Src/Nfc/Desfire/Commands/GetValueCommand.cpp`
- `Src/Nfc/Desfire/Commands/GetCardUidCommand.cpp`
- `Src/Nfc/Desfire/Commands/SetConfigurationCommand.cpp`
- `Src/Nfc/Desfire/Commands/ReadRecordsCommand.cpp`
- `Src/Nfc/Desfire/Commands/WriteRecordCommand.cpp`

Observed:
- Multiple command-local implementations of:
  - `resolveSessionCipher(...)`
  - `calculateCrc16(...)`
  - `calculateCrc32Desfire(...)`
- Yet `Include/Nfc/Desfire/Commands/ValueOperationCryptoUtils.h` already provides shared versions of these.

Risk:
- Algorithm drift (especially CRC variants and session-key interpretation).

---

### 6. AES-CMAC implementation exists both in utility and locally in `GetValueCommand`
Files:
- Utility: `Include/Nfc/Desfire/Commands/ValueOperationCryptoUtils.h`
- Local duplicate: `Src/Nfc/Desfire/Commands/GetValueCommand.cpp`

Observed:
- `GetValueCommand.cpp` contains local AES-CMAC primitives:
  - subkey derivation
  - block pad/xor
  - CMAC calculation
- Functionality overlaps with `valueop_detail::calculateAesCmac(...)`.

Risk:
- Subtle cryptographic behavior mismatch if one implementation changes.

---

### 7. LE packing helpers are repeated in many commands
Repeated helpers:
- `appendLe16`
- `appendLe24`
- `appendLe32`

Files include:
- `CreditCommand.cpp`
- `DebitCommand.cpp`
- `LimitedCreditCommand.cpp`
- `CreateValueFileCommand.cpp`
- `ReadDataCommand.cpp`
- `WriteDataCommand.cpp`
- `ReadRecordsCommand.cpp`
- `WriteRecordCommand.cpp`
- `ChangeKeyCommand.cpp`
- `ChangeKeySettingsCommand.cpp`
- `ChangeFileSettingsCommand.cpp`
- `SetConfigurationCommand.cpp`

Risk:
- Low per-command risk, but noisy duplication and unnecessary code volume.

---

### 8. Response parsing boilerplate is repeated almost everywhere
Common pattern:
- check `response.empty()`
- `result.statusCode = response[0]`
- copy trailing bytes to `result.data`
- success/error mapping
- set completion flag/stage

Appears in most commands in `Src/Nfc/Desfire/Commands`.

Risk:
- Inconsistent status/data handling edge cases across commands.

---

### 9. Non-command standalone tool is present in command folder
File:
- `Src/Nfc/Desfire/Commands/changekey_command.cpp`

Observed:
- Standalone Windows utility (not command class, not built by command CMake list).
- Different coding style and purpose than other command implementations.

Risk:
- Confusion during maintenance and static review (looks like production command code but is not part of build).

## Recommended Refactor Plan (Low-Risk, Incremental)

### Phase 1: Shared crypto helper consolidation
1. Extend `ValueOperationCryptoUtils.h` with reusable helpers for plain authenticated commands:
   - `deriveAesPlainRequestIv(context, messageBytes...)`
   - `verifyAesPlainResponseAndDeriveNextIv(response, context, requestIv)`
2. Replace duplicated local CMAC/IV helpers in:
   - `ClearRecordFileCommand.cpp`
   - `DeleteFileCommand.cpp`
   - `CommitTransactionCommand.cpp`
   - `FormatPiccCommand.cpp`
   - `FreeMemoryCommand.cpp`
   - `GetFileSettingsCommand.cpp`
   - `ReadRecordsCommand.cpp` (AES plain request IV path)

### Phase 2: Unify CRC/session helper usage
1. Remove command-local copies of:
   - `resolveSessionCipher`
   - `calculateCrc16`
   - `calculateCrc32Desfire`
2. Route all these calls through `valueop_detail`.
3. Keep per-command logic only where behavior genuinely differs.

### Phase 3: Introduce common command building blocks
1. Add small internal helpers (or a base class) for:
   - status/data extraction
   - `isComplete/reset` one-shot pattern
   - LE encoders (`appendLe16/24/32`)
2. Add a `value mutation` internal helper used by:
   - `CreditCommand`
   - `DebitCommand`
   - `LimitedCreditCommand`
3. Add `create file common` helper for the two create-file pairs.

### Phase 4: Cleanup and folder hygiene
1. Move `changekey_command.cpp` to tooling/test area (e.g. `tools/` or `WorkingOtherLibs/`), or clearly label/exclude it in docs.
2. Add a short `Docs/` note mapping command crypto paths to shared helper APIs.

## Suggested Priority Order
1. CMAC/IV helper unification (highest correctness impact).
2. CRC/session helper unification.
3. Value-operation triplet consolidation.
4. Create-file pair dedup.
5. Boilerplate parser/base cleanup.

## Expected Benefits
- Lower crypto regression risk (single source of truth for CMAC/CRC/session decisions).
- Faster command additions (less copy-paste scaffolding).
- Easier review/debugging (fewer near-identical files with slight drift).
- Clearer ownership of secure-messaging behavior.


