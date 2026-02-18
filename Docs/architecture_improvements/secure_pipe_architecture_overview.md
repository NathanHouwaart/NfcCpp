# Secure Pipe Architecture Overview

## Purpose
This document expands section **F** from `Docs/desfire_architecture_refactor_report.md` and shows what each architecture direction would look like in practice.

## Current State
The codebase currently uses a hybrid model:

1. **Active path**: command-centric secure messaging.
2. **Dormant path**: `ISecurePipe` + `PlainPipe` + `MacPipe` + `EncPipe` + `DesfireCard::wrapRequest/unwrapResponse`.

The active path is working and already handles AES/3K3DES/2K3DES/DES session behavior in command code and shared crypto helpers.

## Option A: Remove Dormant Secure-Pipe Seams (Short-Term Cleanup)

## What this architecture looks like
- `DesfireCard` remains a thin orchestrator over command execution.
- Commands and shared helper utilities remain the only secure messaging implementation.
- No unused pipe classes, no dead API surface.

## File-level changes
1. Remove pipe includes and members from `Include/Nfc/Desfire/DesfireCard.h`.
2. Remove unimplemented pipe methods from `Include/Nfc/Desfire/DesfireCard.h` and `Src/Nfc/Desfire/DesfireCard.cpp`:
- `wrapRequest(...)`
- `unwrapResponse(...)`
3. Remove these unused files:
- `Include/Nfc/Desfire/ISecurePipe.h`
- `Include/Nfc/Desfire/PlainPipe.h`
- `Include/Nfc/Desfire/MacPipe.h`
- `Include/Nfc/Desfire/EncPipe.h`
- `Src/Nfc/Desfire/PlainPipe.cpp`
- `Src/Nfc/Desfire/MacPipe.cpp`
- `Src/Nfc/Desfire/EncPipe.cpp`
4. Remove deleted sources from `Src/Nfc/Desfire/CMakeLists.txt`.

## Behavioral impact
- No intended behavior changes.
- Build graph and public API become smaller.
- Lower chance of future confusion about "which secure path is real."

## Effort and risk
- Effort: low.
- Runtime risk: low.
- Regression risk: mostly compile-time and include/dependency cleanup.

## Validation checklist
1. Full build passes.
2. Existing DESFire examples pass (especially auth, read/write, value operations, session drift).
3. No public references remain to removed pipe types.

## Option B: Fully Migrate to Secure-Pipe Architecture (Long-Term Rewrite)

## What this architecture looks like
- Commands define payload semantics and security intent.
- A central secure-pipe layer performs:
- command wrapping
- encryption/MAC generation
- response MAC/CMAC verification
- response decrypt/unpad
- IV progression
- Commands stop performing cryptographic transformations directly.

## Required design changes
1. Extend command contract with security metadata.
2. Define one canonical secure-messaging state machine for AES, ISO 3DES, legacy DES.
3. Move per-command crypto blocks from command files into secure-pipe implementation.
4. Keep command files focused on:
- payload encode
- payload decode
- domain-level validation

## Likely migration phases
1. Introduce security metadata model and central secure policy interfaces.
2. Implement plain + authenticated-plain paths in secure-pipe.
3. Migrate a narrow vertical slice first:
- `GetValue`
- `Credit/Debit/LimitedCredit`
- `CommitTransaction`
4. Migrate chunked paths:
- `ReadData` / `WriteData`
- `ReadRecords` / `WriteRecord`
5. Remove now-redundant command-local crypto branches.

## Behavioral impact
- High internal change volume.
- Potentially better consistency long-term.
- High risk during migration if both systems coexist for too long.

## Effort and risk
- Effort: high.
- Runtime risk: high during migration.
- Regression risk: significant across all auth modes and file operation families.

## Decision Matrix

| Dimension | Option A (Remove Dead Seams) | Option B (Full Secure-Pipe Migration) |
|---|---|---|
| Initial effort | Low | High |
| Time to value | Fast | Slow |
| Runtime risk | Low | High |
| Architectural purity | Medium | High |
| Near-term maintainability | High | Medium (until migration completes) |

## Recommended Path
Use a two-step strategy:

1. Execute **Option A now** to remove dead code and reduce ambiguity.
2. Revisit **Option B only after**:
- lifecycle and chunk-engine abstractions are stabilized,
- secure-messaging behavior is covered by stronger automated tests.

This sequence keeps the working system stable while creating room for a cleaner long-term architecture.

## Suggested Next Actions
1. Do Option A as a standalone PR with no behavior changes.
2. Add coverage for IV/CMAC progression chains in unit/integration tests.
3. If Option B is still desired, create an RFC with an explicit migration boundary and command-by-command rollout plan.
