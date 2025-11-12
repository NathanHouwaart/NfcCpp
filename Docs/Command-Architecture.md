# PN532 Command Architecture

## Overview
This document describes the clean, layered architecture for PN532 command execution.

## Architecture Layers

### 1. Protocol Layer (`Pn532Driver::transceive`)
**Purpose**: Handle low-level PN532 protocol communication
- **Input**: `CommandRequest` (command code + payload + timeout)
- **Output**: `Pn532ResponseFrame` (raw protocol frame)
- **Responsibilities**:
  - Build PN532 frame from request
  - Send frame to hardware bus
  - Wait for ACK
  - Read response frame
  - Validate frame structure and checksums
  - Verify command code matches

### 2. Orchestration Layer (`Pn532Driver::executeCommand`)
**Purpose**: Orchestrate the complete command execution flow
- **Input**: `IPn532Command&` (command interface)
- **Output**: `CommandResponse` (parsed response)
- **Responsibilities**:
  - Call `command.buildRequest()` to get request
  - Call `transceive()` to send/receive frames
  - Call `command.parseResponse()` to parse frame
  - Handle errors at each step

### 3. Application Layer (`IPn532Command` implementations)
**Purpose**: Implement command-specific logic
- **Examples**: `GetFirmwareVersion`, `InListPassiveTarget`, etc.
- **Responsibilities**:
  - Build command-specific requests
  - Parse command-specific responses
  - Cache/store command results

## Data Flow

```
User Code
    ↓
    ↓ calls executeCommand(GetFirmwareVersion)
    ↓
Pn532Driver::executeCommand
    ↓
    ├─→ command.buildRequest() → CommandRequest
    ↓
    ├─→ transceive(CommandRequest)
    │       ↓
    │       ├─→ Pn532RequestFrame::build() → frame bytes
    │       ├─→ sendCommand() → hardware
    │       ├─→ wait & read ACK
    │       ├─→ read response bytes
    │       └─→ parseResponseFrame() → Pn532ResponseFrame
    ↓
    └─→ command.parseResponse(Pn532ResponseFrame) → CommandResponse
    ↓
User Code (gets CommandResponse)
```

## Key Types

### CommandRequest
- Command code
- Payload data
- Timeout
- Created by: `IPn532Command::buildRequest()`

### Pn532ResponseFrame
- Raw protocol-level response
- Command code (validated)
- Payload data (after protocol parsing)
- Created by: `Pn532Driver::parseResponseFrame()`

### CommandResponse
- High-level command response
- Command code
- Parsed payload
- Created by: `IPn532Command::parseResponse()`

## Example: GetFirmwareVersion

```cpp
// 1. Application Layer - Build Request
CommandRequest GetFirmwareVersion::buildRequest() {
    etl::vector<uint8_t, 1> payload; // Empty payload
    return createCommandRequest(0x02, payload);
}

// 2. Protocol Layer - Transceive
Pn532Driver::transceive(request)
    → Sends: [00 00 FF 02 FE D4 02 2A 00]
    ← Receives: [00 00 FF 00 FF 00] (ACK)
    ← Receives: [00 00 FF 06 FA D5 03 32 01 06 07 E8 00] (Response)
    → Returns: Pn532ResponseFrame(cmd=0x03, payload=[32 01 06 07])

// 3. Application Layer - Parse Response
CommandResponse GetFirmwareVersion::parseResponse(frame) {
    cachedInfo.ic = frame.data()[0];      // 0x32
    cachedInfo.ver = frame.data()[1];     // 0x01
    cachedInfo.rev = frame.data()[2];     // 0x06
    cachedInfo.support = frame.data()[3]; // 0x07
    return createCommandResponse(0x03, frame.data());
}
```

## Benefits

✅ **Clear Separation of Concerns**: Each layer has a single responsibility
✅ **Reusable**: Protocol layer works for all commands
✅ **Testable**: Each layer can be tested independently
✅ **Type-Safe**: Strong typing at each boundary
✅ **Error Handling**: `etl::expected` at each step for clean error propagation
✅ **Extensible**: Easy to add new commands by implementing `IPn532Command`

## Removed/Deprecated

- ❌ `CommandResult` - Redundant, replaced by `CommandResponse`
- ⚠️ `Pn532Response` - Marked as legacy, use `CommandResponse` instead
