# SecureMessagingPolicy Migration Checklist

## Goal
Fully centralize DESFire secure-messaging behavior (cipher selection, request protection, response verification, IV progression, legacy reset policy) behind `SecureMessagingPolicy`, and remove command-level crypto branching.

## Current Status
- [x] Added `SecureMessagingPolicy` skeleton and plain-command IV progression API.
- [x] Migrated authenticated-plain IV update for:
  - `CommitTransactionCommand`
  - `FormatPiccCommand`
  - `DeleteFileCommand`
  - `ClearRecordFileCommand`
- [x] Added value-operation API surface in policy:
  - request protection (`protectValueOperationRequest`)
  - response IV progression (`updateContextIvForValueOperationResponse`)
- [x] Migrated value mutation command family to policy-driven secure messaging:
  - `CreditCommand`
  - `DebitCommand`
  - `LimitedCreditCommand`
  - shared helper `ValueMutationCommandUtils`

## Remaining Migration Work

## Phase 2: Read/Write Data + Records (highest risk/ROI)
- [x] Migrate `WriteDataCommand` secure messaging path to policy.
- [x] Migrate `ReadDataCommand` secure messaging path to policy.
- [x] Migrate `WriteRecordCommand` secure messaging path to policy.
- [x] Migrate `ReadRecordsCommand` secure messaging path to policy.
- [x] Move authenticated-plain payload verification logic out of command files and into policy helper(s).
- [x] Remove command-local heuristic MAC trimming when authenticated verification is available.

### Phase 2 candidate files
- [x] `Src/Nfc/Desfire/Commands/WriteDataCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/ReadDataCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/WriteRecordCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/ReadRecordsCommand.cpp`

## Phase 3: Other Secure Commands
- [x] Migrate `GetValueCommand` IV/MAC handling to policy.
- [x] Migrate `GetCardUidCommand` encrypted-response IV progression to policy.
- [x] Migrate `FreeMemoryCommand` authenticated-plain verification to policy.
- [x] Migrate `GetFileSettingsCommand` authenticated-plain verification to policy.
- [x] Migrate `ChangeKey*` command secure-messaging branches to policy.
- [x] Migrate `ChangeFileSettingsCommand` and `SetConfigurationCommand` secure messaging to policy.
- [x] Migrate remaining authenticated-plain commands that still perform manual CMAC verification.

### Phase 3 candidate files
- [x] `Src/Nfc/Desfire/Commands/GetValueCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/GetCardUidCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/FreeMemoryCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/GetFileSettingsCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/ChangeKeyCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/ChangeKeySettingsCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/ChangeFileSettingsCommand.cpp`
- [x] `Src/Nfc/Desfire/Commands/SetConfigurationCommand.cpp`

## Phase 4: Consolidation and Cleanup
- [x] Replace direct command usage of `ValueOperationCryptoUtils.h` with policy-only calls.
  - Completed for: `WriteData`, `ReadData`, `WriteRecord`, `ReadRecords`, `GetValue`, `GetCardUid`, `GetFileSettings`, `ChangeKeyCommand`, `ChangeKeySettingsCommand`, `ChangeFileSettingsCommand`, `SetConfigurationCommand`.
  - Remaining utility-level usage: `SecureMessagingPolicy` internals.
- [x] Reduce `ValueOperationCryptoUtils.h` to low-level primitives or move to `.cpp` private implementation.
- [x] Remove duplicate helper code that becomes dead after policy migration.
- [x] Add targeted unit tests for policy:
  - AES path
  - ISO 3K3DES path
  - ISO 2K3DES path
  - Legacy DES/2K3DES reset behavior

## Phase 5: Pipe Decision (separate architectural decision)
- [ ] Decide and document one direction:
  - keep command-centric model + remove dead pipes, or
  - complete pipe model and make policy the shared engine underneath.
- [ ] Remove dead seam (`wrapRequest` / `unwrapResponse`) if not used.
- [ ] Remove or revive `PlainPipe` / `MacPipe` / `EncPipe` consistently.

## Acceptance Criteria
- [ ] No command performs cipher-specific IV/CMAC branching directly.
- [ ] `SecureMessagingPolicy` is the single authoritative secure-messaging progression point.
- [x] Full hardware flow passes:
  - format
  - app recreate
  - baseline drift
  - drift mode
- [x] Drift tests are stable across AES, 3K3DES, 2K3DES, and legacy DES scenarios.
