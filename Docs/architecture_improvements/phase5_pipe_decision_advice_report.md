# Phase 5 Pipe Decision Advice Report

## Scope
This report evaluates **Phase 5** from `Docs/architecture_improvements/secure_messaging_policy_migration_checklist.md`:
- decide architecture direction,
- remove dead seam (`wrapRequest` / `unwrapResponse`) if unused,
- remove or revive `PlainPipe` / `MacPipe` / `EncPipe`.

## Current Reality (Code Evidence)
- `DesfireCard::executeCommand(...)` runs command-centric flow directly:
  - `command.buildRequest(...)`
  - `wire->wrap(...)`
  - `transceiver.transceive(...)`
  - `command.parseResponse(...)`
- `DesfireCard::wrapRequest(...)` and `DesfireCard::unwrapResponse(...)` are present but return `NotSupported`.
- `PlainPipe`, `MacPipe`, `EncPipe` are compiled but not used by any runtime path.
- `plainPipe/macPipe/encPipe` members exist in `DesfireCard` but are never initialized or used.
- Secure messaging behavior now lives in commands + `SecureMessagingPolicy`.

Conclusion: the pipe model is currently **dead surface area**, not an active subsystem.

## Option Analysis

## Option A: Keep command-centric model, remove dead pipes/seams
Pros:
- Lowest risk to working AES/3K3DES/2K3DES/DES behavior.
- Matches current architecture and tests.
- Reduces cognitive load and API confusion.
- Small, reviewable change set.

Cons:
- Gives up near-term "clean pipe abstraction" vision.

## Option B: Fully migrate to pipe architecture
Pros:
- Potentially cleaner layering long-term.
- Could centralize request/response security transforms in one runtime path.

Cons:
- Large rewrite with high regression risk.
- Duplicates/competes with already-working command + policy model.
- Requires redesign of command contracts before value is realized.

## Recommendation
Choose **Option A now**:
- keep command-centric execution,
- keep `SecureMessagingPolicy` as central crypto progression service,
- remove dead pipe/seam code.

Rationale: this aligns with the current stable implementation and avoids destabilizing secure messaging after recent fixes.

## Proposed Phase 5 Execution Plan

1. **Internal cleanup (safe)**
- Remove unused pipe includes from `Src/Nfc/Desfire/DesfireCard.cpp`.
- Remove unused pipe pointer members from `Include/Nfc/Desfire/DesfireCard.h`.
- Remove `PlainPipe.cpp`, `MacPipe.cpp`, `EncPipe.cpp` from `Src/Nfc/Desfire/CMakeLists.txt`.
- Delete:
  - `Include/Nfc/Desfire/ISecurePipe.h`
  - `Include/Nfc/Desfire/PlainPipe.h`
  - `Include/Nfc/Desfire/MacPipe.h`
  - `Include/Nfc/Desfire/EncPipe.h`
  - `Src/Nfc/Desfire/PlainPipe.cpp`
  - `Src/Nfc/Desfire/MacPipe.cpp`
  - `Src/Nfc/Desfire/EncPipe.cpp`

2. **Public API seam decision**
- If backward compatibility is required short-term:
  - keep `wrapRequest/unwrapResponse` temporarily, mark deprecated, document unsupported status.
- If API cleanup is allowed now:
  - remove `wrapRequest/unwrapResponse` from `DesfireCard` header and source.

3. **Validation**
- Full Debug build.
- Unit tests.
- Hardware smoke:
  - format,
  - session drift baseline,
  - session drift drift-mode.

## Effort Estimate
- Option A cleanup only: ~0.5 day.
- Option A + public API removal + docs/tests update: ~1 day.
- Option B full migration: multi-week refactor.

## Risks and Mitigations
- Risk: external code may call `wrapRequest/unwrapResponse`.
  - Mitigation: deprecate for one release before removal, or remove only if no external consumers.
- Risk: accidental include/link breakage after file deletion.
  - Mitigation: one PR with build + unit + hardware validation gate.

## Final Advice
For this codebase state, **do not start a full pipe migration**.  
Complete Phase 5 by removing dead seams and keeping the command-centric + `SecureMessagingPolicy` architecture as the single production path.
