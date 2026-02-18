# DESFire Secure Messaging Layout and IV/CMAC Report

Date: 2026-02-17

## Scope

This report covers:

- The current status of `EncPipe.cpp`, `MacPipe.cpp`, and `PlainPipe.cpp`
- Whether those files should be used to "better layout" the code now
- Current IV and CMAC progression behavior in the working implementation
- Refactor recommendations
- AI-ready rules so generated code is consistent and safe

## Executive Conclusion

The current working command-centric implementation is better than the current `EncPipe/MacPipe/PlainPipe` implementation.

Reason:

- The pipe files are currently stubs and are not integrated into the execution path.
- Critical secure messaging behavior (IV progression, CMAC verification, CRC handling, legacy-vs-ISO rules) lives in command implementations and helper utilities.
- Enabling the current pipe stubs would reduce correctness, not improve it.

Short answer to your question:

- Keep current behavior.
- Do not route traffic through the three pipe files as they are today.
- Refactor by extracting shared crypto/session progression logic, not by switching to the current pipe stubs.

## Current State (What Is True in Code)

### Pipe files

- `Src/Nfc/Desfire/PlainPipe.cpp`: pass-through only
- `Src/Nfc/Desfire/MacPipe.cpp`: TODO, no real MAC handling
- `Src/Nfc/Desfire/EncPipe.cpp`: TODO, no real encryption/MAC handling

### DesfireCard integration

- `DesfireCard` has `plainPipe`, `macPipe`, `encPipe` members, but they are `nullptr` and not initialized.
- `DesfireCard::wrapRequest` and `DesfireCard::unwrapResponse` are TODO and return `NotSupported`.
- `DesfireCard::executeCommand` uses command `buildRequest/parseResponse` and `wire->wrap(...)` directly.

Result:

- Secure messaging correctness currently depends on command code + helper utilities, not pipe classes.

## IV/CMAC Progression Analysis (Current Working Behavior)

### Session establishment and reset

- Authentication sets:
  - `context.sessionKeyEnc`
  - `context.sessionKeyMac`
  - `context.iv` to zero block of active block size
  - `context.authScheme` (`Legacy`, `Iso`, `Aes`)
- Session reset happens on app/key boundary changes (for example `SelectApplication`, and key-change flows).

### Progression model now in use

- AES and ISO 3DES/2K3DES:
  - Request-side IV derivation uses CMAC over command header/message.
  - Response-side integrity checks verify truncated CMAC where applicable.
  - Context IV advances from computed CMAC (not guessed from transport bytes).
- Legacy DES and legacy 2K3DES:
  - Uses legacy command-local chaining rules.
  - IV is intentionally reset between commands in this mode.

This is why your latest runs complete: each session family now follows its own consistent progression rules.

### Where this logic is implemented

Core context and session state:

- `Include/Nfc/Desfire/DesfireContext.h`
- `Src/Nfc/Desfire/Commands/AuthenticateCommand.cpp`

Shared crypto/progression helpers:

- `Include/Nfc/Desfire/Commands/ValueOperationCryptoUtils.h`
- `Include/Nfc/Desfire/Commands/ValueMutationCommandUtils.h`

Command-specific progression and parsing:

- `Src/Nfc/Desfire/Commands/WriteDataCommand.cpp`
- `Src/Nfc/Desfire/Commands/WriteRecordCommand.cpp`
- `Src/Nfc/Desfire/Commands/ReadDataCommand.cpp`
- `Src/Nfc/Desfire/Commands/ReadRecordsCommand.cpp`
- `Src/Nfc/Desfire/Commands/GetValueCommand.cpp`
- `Src/Nfc/Desfire/Commands/CreditCommand.cpp`
- `Src/Nfc/Desfire/Commands/DebitCommand.cpp`
- `Src/Nfc/Desfire/Commands/LimitedCreditCommand.cpp`
- `Src/Nfc/Desfire/Commands/CommitTransactionCommand.cpp`

## Cleanliness Assessment

### What is good now

- Behavior is correct across AES, 3K3DES, 2K3DES ISO, and legacy DES paths.
- `SessionAuthScheme` is explicit and actively used.
- Shared helpers exist for many tricky CMAC/IV operations.
- Error mapping is strict (`IntegrityError`, `InvalidResponse`, etc.).

### What is still messy

- Crypto/session progression logic is still duplicated in multiple command files.
- Similar command pairs (`WriteData`/`WriteRecord`, `ReadData`/`ReadRecords`) still duplicate substantial secure messaging logic.
- Some commands use central helpers heavily; others still contain local complex fallback/decode logic.
- Pipe abstraction exists but is not authoritative; this creates architectural confusion.

## Recommendation on the Three Pipe Files

### Recommendation now

Do not use `EncPipe`, `MacPipe`, `PlainPipe` in their current form.

### What to do instead

Option A (recommended):

- Keep command-centric flow.
- Consolidate secure messaging helpers further.
- Remove or deprecate unused pipe abstraction to reduce confusion.

Option B (future, only if redesigned):

- Replace current pipe classes with a real `SecureMessagingEngine` that owns:
  - request protection
  - response verification/decoding
  - IV progression updates
- Commands provide metadata and payload only.

If you keep the three files, they should be clearly marked deprecated/unwired until they are fully implemented and adopted.

## AI-Ready Coding Rules (Use This For Code Generation)

Use these rules as hard constraints when generating secure messaging code:

1. Never update `context.iv` before response integrity is validated.
2. Determine crypto behavior from both:
   - `context.authScheme`
   - session key size / resolved session cipher
3. For AES and ISO-style 3DES sessions:
   - derive request IV from CMAC(command-message)
   - verify response CMAC when present
   - advance context IV from computed CMAC bytes
4. For legacy DES and legacy 2K3DES:
   - use legacy command-local chaining rules
   - reset context IV to zero 8-byte block at command boundary
5. Do not treat `0xAF` as failure in multi-frame flows; continue stage machine.
6. Parse/decrypt only after status handling is correct.
7. On any integrity/shape mismatch, return `InvalidResponse` or `IntegrityError` immediately.
8. Keep CRC checks mode-aware:
   - legacy paths can need CRC16
   - ISO/AES paths typically use CRC32/CMAC checks
9. Keep transport framing (`wire->wrap`) separate from DESFire secure messaging semantics.
10. Add regression coverage for each auth family and command type before merging.

## Suggested Refactor Plan

### Phase 1 (safe cleanup, low risk)

- Extract duplicated IV update branches into one shared helper.
- Extract duplicated encrypted payload builders shared by write commands.
- Keep command behavior unchanged; only deduplicate.

### Phase 2 (read-path consolidation)

- Unify encrypted read decode/CMAC verification flow shared by `ReadData`, `ReadRecords`, `GetValue`.
- Keep per-command parsing details (sizes/layout) separate from secure envelope handling.

### Phase 3 (optional architecture upgrade)

- If desired, introduce a real secure messaging engine and remove dead pipe abstraction.
- Do this only after test coverage locks current behavior.

## Definition of Done for Future AI-Generated Changes

A change touching secure messaging is acceptable only if:

- AES, 3K3DES, 2K3DES ISO, and legacy DES scripted demos all pass.
- No regression in previously fixed steps:
  - ChangeKeySettings flows
  - WriteData/ReadData flows
  - Value ops + commit + post-read
  - WriteRecord/ReadRecords flows
- IV/CMAC updates are centralized or proven equivalent by tests/log comparison.
- No new dead abstractions are introduced.

