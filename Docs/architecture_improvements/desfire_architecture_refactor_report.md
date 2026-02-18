# DESFire Command Architecture Refactor Report

## Scope
This report analyzes the current DESFire command layer and identifies recurring patterns that can be abstracted to reduce duplication, lower bug risk, and make future command work easier.

## Current Snapshot
- Command implementations in `Src/Nfc/Desfire/Commands`: 34 files.
- Repeated `result.statusCode = response[0]` parsing pattern: 26 files.
- Commands with `bool complete` lifecycle: 15 headers.
- Commands with custom `Stage` state machine lifecycle: 18 headers.
- `resolveSessionCipher(...)` usage scattered across commands: 24 references.
- Manual `context.iv.clear()` update points: 23 references.

## Key Findings

### 1) Lifecycle boilerplate is repeated across most commands
- Evidence:
  - `Src/Nfc/Desfire/Commands/CreateApplicationCommand.cpp`
  - `Src/Nfc/Desfire/Commands/DeleteApplicationCommand.cpp`
  - `Src/Nfc/Desfire/Commands/FormatPiccCommand.cpp`
  - `Src/Nfc/Desfire/Commands/ClearRecordFileCommand.cpp`
- Pattern repeated:
  - guard invalid state
  - build request
  - parse empty response check
  - copy status + payload into `DesfireResult`
  - set complete/stage

### 2) Chunked read/write commands duplicate secure messaging logic heavily
- Evidence:
  - `Src/Nfc/Desfire/Commands/WriteDataCommand.cpp`
  - `Src/Nfc/Desfire/Commands/WriteRecordCommand.cpp`
  - `Src/Nfc/Desfire/Commands/ReadDataCommand.cpp`
  - `Src/Nfc/Desfire/Commands/ReadRecordsCommand.cpp`
- Similar responsibilities are implemented 2-4 times:
  - chunk planning
  - command-local state transitions
  - communication settings resolution
  - session-cipher detection
  - IV update behavior
  - trimming/decoding authenticated payloads

### 3) Secure messaging policy is split between command code and helper headers
- Evidence:
  - `Include/Nfc/Desfire/Commands/ValueOperationCryptoUtils.h`
  - `Include/Nfc/Desfire/Commands/ValueMutationCommandUtils.h`
  - command-local crypto logic in `ChangeKey*`, `Read*`, `Write*`, `GetValue`
- Result:
  - command classes must know too much about IV/CMAC progression
  - easy to miss one cipher branch (the recent 2K3DES commit-transaction drift bug came from exactly this class of issue)

### 4) Authenticated plain-response handling is inconsistent across commands
- Some commands verify authenticated responses (AES/TKTDES paths).
- Some commands trim trailing MAC/CMAC heuristically without verification.
- Evidence:
  - Heuristic trimming:
    - `Src/Nfc/Desfire/Commands/GetApplicationIdsCommand.cpp`
    - `Src/Nfc/Desfire/Commands/GetFileIdsCommand.cpp`
  - Explicit verification paths:
    - `Src/Nfc/Desfire/Commands/FreeMemoryCommand.cpp`
    - `Src/Nfc/Desfire/Commands/GetFileSettingsCommand.cpp`

### 5) `DesfireCard` has high orchestration duplication and old dead seams
- Evidence:
  - Many repetitive wrappers in `Src/Nfc/Desfire/DesfireCard.cpp` returning `executeCommand(command)`.
  - Legacy unused pipe seam still present:
    - includes and members for `PlainPipe`, `MacPipe`, `EncPipe` in `Include/Nfc/Desfire/DesfireCard.h`
    - unimplemented methods `wrapRequest` / `unwrapResponse` in `Src/Nfc/Desfire/DesfireCard.cpp`
    - TODO-only implementations in `Src/Nfc/Desfire/PlainPipe.cpp`, `Src/Nfc/Desfire/MacPipe.cpp`, `Src/Nfc/Desfire/EncPipe.cpp`

### 6) Utility progress exists but is not generalized yet
- Good direction already present:
  - `Include/Nfc/Desfire/Commands/CreateFileCommandUtils.h`
  - `Include/Nfc/Desfire/Commands/ValueMutationCommandUtils.h`
- These prove abstraction works, but scope is still narrow.

## Recommended Architectural Improvements

## A) Introduce command base classes for lifecycle
- Add:
  - `SingleFrameCommandBase` for complete-once commands.
  - `MultiFrameCommandBase` for staged commands.
- Centralize:
  - invalid-state checks
  - `response.empty()` handling
  - standard status/payload extraction
- Keep only command-specific payload encode/decode in derived classes.

## B) Extract a shared `SecureMessagingPolicy` service
- Move cipher/IV/CMAC progression logic behind one interface:
  - resolve session cipher
  - derive request IV
  - verify response MAC/CMAC
  - derive next IV
  - legacy DES/2K3DES reset behavior
- Commands should request policy operations, not implement cryptographic branching directly.

### What this would look like
Define one service (or small set of services) that owns session-security behavior:
- Input:
  - current `DesfireContext`
  - command descriptor (`INS`, comm mode, auth mode hints)
  - request/response payload bytes
- Output:
  - transformed payload (encrypted/decrypted or plain)
  - verification result (MAC/CMAC ok, CRC policy result)
  - next IV/session progression update

### Proposed minimal interface
```cpp
struct SecurePolicyRequestMeta {
    uint8_t ins;
    uint8_t communicationSettings; // 0x00,0x01,0x03
    bool allowAdditionalFrame;
};

struct SecurePolicyResult {
    etl::vector<uint8_t, 256> payload;
    etl::vector<uint8_t, 16> nextIv;
    bool verified;
};

class SecureMessagingPolicy {
public:
    SessionCipher resolveCipher(const DesfireContext& ctx) const;
    etl::expected<SecurePolicyResult, error::Error> protectRequest(
        const DesfireContext& ctx,
        const SecurePolicyRequestMeta& meta,
        const etl::ivector<uint8_t>& plainPayload) const;
    etl::expected<SecurePolicyResult, error::Error> unprotectResponse(
        const DesfireContext& ctx,
        const SecurePolicyRequestMeta& meta,
        const etl::ivector<uint8_t>& rawResponse,
        const etl::ivector<uint8_t>& requestState) const;
};
```

### Command responsibilities after this refactor
- Commands keep:
  - domain validation
  - domain encoding/decoding (file ids, offsets, values, record sizes)
  - stage progression (`Initial`, `AdditionalFrame`, `Complete`)
- Commands lose:
  - cipher selection branches
  - repeated IV derivation/update code
  - repeated response MAC/CMAC verification branches

### Relationship to pipes (`ISecurePipe`, `PlainPipe`, `MacPipe`, `EncPipe`)
This option is **related but not dependent** on pipes.

- It can be used **without pipes**:
  - keep current command-centric transport flow
  - call `SecureMessagingPolicy` from commands/helpers
  - remove duplicated crypto branches immediately

- It can also be used **with pipes**:
  - pipes become transport adapters
  - `SecureMessagingPolicy` becomes the shared crypto/session engine underneath

So conceptually:
- `SecureMessagingPolicy` = security/session rules engine
- `Pipe` abstraction = transport wrapping layer

They are orthogonal. You can adopt policy first and decide pipe direction later.

### Recommendation
Implement `SecureMessagingPolicy` first in the current command-centric model.
- This yields immediate consistency and bug reduction.
- It reduces migration risk if you later choose to fully revive pipe architecture.

## C) Create generic chunk engines
- Add:
  - `ChunkedWriteEngine` used by `WriteDataCommand` and `WriteRecordCommand`.
  - `ChunkedReadEngine` used by `ReadDataCommand` and `ReadRecordsCommand`.
- Strategy inputs:
  - command code
  - address encoder (offset/record fields)
  - payload decode/encode callback
  - expected response shaping
- This removes the largest repeated blocks in current command implementations.

## D) Standardize authenticated plain response parsing
- Add one reusable helper:
  - `AuthenticatedPlainResponseParser`
- Responsibilities:
  - parse status+payload
  - verify trailing MAC/CMAC when authenticated (AES + TKTDES)
  - reject inconsistent payload lengths
  - update IV consistently
- Replace command-local heuristic trimming where possible.

## E) Simplify `DesfireCard` facade
- Keep `executeCommand()` as core.
- Add small generic helpers:
  - `executeVoid<TCommand>(...)`
  - `executeValue<TCommand, TValue>(..., extractor)`
- This removes repeated "construct command -> execute -> unwrap command getter" wrappers.

## F) Decide on secure-pipe architecture explicitly
- Current state is hybrid: direct command-level security handling + dormant pipe abstraction.
- Recommendation:
  - either fully remove `ISecurePipe`/`PlainPipe`/`MacPipe`/`EncPipe` and `wrapRequest/unwrapResponse`,
  - or fully migrate to it.
- Current command-centric model is already working; completing pipe migration would be a large rewrite. Short term, removal of dead seams is cleaner.

## G) Move heavy crypto helper implementation out of headers
- `ValueOperationCryptoUtils.h` and `ValueMutationCommandUtils.h` are large inline-heavy utility units.
- Consider `.cpp` implementation with a small public interface to:
  - reduce compile times
  - reduce accidental duplication/inconsistency
  - improve testability via narrower APIs

## Priority Refactor Plan

1. **Low risk / high ROI**
- Add lifecycle base class and shared result parsing helper.
- Remove repeated status/payload parse loops.

2. **Medium risk / high ROI**
- Add shared secure messaging policy API and migrate `CommitTransaction`, `GetValue`, `ReadData`, `WriteData` first.

3. **Medium-high risk / highest ROI**
- Extract chunk engines and migrate read/write data+record commands.

4. **Cleanup**
- Remove or finalize dormant secure pipe path.
- Normalize naming/file placement (`changekey_command.cpp` currently sits beside production commands but behaves like a standalone tool).

## AI-Oriented Coding Guide (for future generation)
- Prefer extending shared utilities over adding command-local crypto branches.
- New command template should follow:
  - validate options
  - build payload only
  - delegate secure messaging to policy
  - delegate status/payload parsing to base/helper
  - update command-specific domain state only
- Any new authenticated command must include tests for:
  - AES
  - 3K3DES (ISO)
  - 2K3DES (ISO and legacy where applicable)
  - IV progression across command chains
- Avoid heuristic MAC trimming unless parser policy explicitly allows and logs fallback mode.

## Suggested Next Concrete Step
Implement `SingleFrameCommandBase` + `parseSimpleStatusResponse` and migrate these first:
- `CreateApplicationCommand`
- `DeleteApplicationCommand`
- `DeleteFileCommand`
- `FormatPiccCommand`
- `ClearRecordFileCommand`

This gives immediate duplication reduction with minimal behavior risk, then secure-messaging unification can proceed safely.
