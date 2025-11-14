# DESFire Authentication Implementation

This document describes the DESFire authentication implementation for Legacy (0x0A) and ISO (0x1A) modes.

## Overview

The authentication follows a multi-stage state machine pattern:
1. **Initial**: Send authenticate command with key number
2. **ChallengeSent**: Decrypt card challenge, generate response
3. **ResponseSent**: Verify card's response
4. **Complete**: Authentication successful, session key generated

## Files

### Cryptographic Utilities

**`Include/Utils/DesfireCrypto.h`** and **`Src/Utils/DesfireCrypto.cpp`**

Provides cryptographic functions for DESFire authentication:

- **Rotation functions**: `rotateLeft()`, `rotateRight()`
- **Random generation**: `generateRandom()`
- **DES/3DES encryption/decryption**: ECB and CBC modes
- **Session key generation**: From RndA and RndB with parity bit clearing
- **Byte conversion**: Between byte arrays and uint64_t

### Authentication Command

**`Include/Nfc/Desfire/Commands/AuthenticateCommand.h`** and **`Src/Nfc/Desfire/Commands/AuthenticateCommand.cpp`**

Implements the state machine for DESFire authentication:

```cpp
AuthenticateCommandOptions options;
options.mode = DesfireAuthMode::ISO;  // or DesfireAuthMode::Legacy
options.keyNo = 0;
options.key = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

AuthenticateCommand cmd(options);
```

## Authentication Flow

### Legacy Mode (0x0A)

1. **Send** authenticate command with key number
2. **Receive** 8 bytes encrypted RndB
3. **Decrypt** RndB using 3DES ECB with key
4. **Rotate** RndB right by 1 byte → RndB'
5. **Generate** random 8-byte RndA
6. **Concatenate** RndA || RndB' (16 bytes)
7. **Encrypt** using 3DES CBC with IV=0
8. **Send** encrypted response
9. **Receive** 8 bytes encrypted RndA'
10. **Decrypt** RndA' using 3DES ECB
11. **Rotate** right by 1 byte and **verify** matches original RndA
12. **Generate** session key from RndA and RndB

### ISO Mode (0x1A)

Similar to Legacy but with IV management:

1. **Send** authenticate command with key number
2. **Receive** 8 bytes encrypted RndB
3. **Decrypt** RndB using 3DES CBC with IV=0
4. **Update** IV = encrypted RndB
5. **Rotate** RndB right by 1 byte → RndB'
6. **Generate** random 8-byte RndA
7. **Concatenate** RndA || RndB' (16 bytes)
8. **Encrypt** using 3DES CBC with current IV
9. **Update** IV = last 8 bytes of encrypted response
10. **Send** encrypted response
11. **Receive** 8 bytes encrypted RndA'
12. **Decrypt** RndA' using 3DES CBC with current IV
13. **Rotate** right by 1 byte and **verify** matches original RndA
14. **Generate** session key from RndA and RndB

### Session Key Generation

For DES/2K3DES keys (16 bytes):
```
SessionKey = RndA[0..3] || RndB[0..3] || RndA[4..7] || RndB[4..7]
```

Then clear LSB of each byte (parity bits):
```
SessionKey[i] &= 0xFE
```

## Usage Example

```cpp
#include "Nfc/Desfire/DesfireCard.h"
#include "Nfc/Desfire/DesfireAuthMode.h"

// Assume we have a DesfireCard instance
DesfireCard card(transceiver);

// Select master application (AID 0x000000)
etl::array<uint8_t, 3> masterAid = {0x00, 0x00, 0x00};
auto result = card.selectApplication(masterAid);
if (!result) {
    // Handle error
}

// Prepare authentication key (factory default: all zeros)
etl::vector<uint8_t, 24> key;
for (int i = 0; i < 16; ++i) {
    key.push_back(0x00);
}

// Authenticate with key 0 using ISO mode
result = card.authenticate(0, key, DesfireAuthMode::ISO);
if (!result) {
    // Handle authentication failure
}

// Card is now authenticated and session key is established
// Further commands will use encrypted communication
```

## Dependencies

- **cppDes**: DES/3DES implementation from external/cppDes
  - `DES`: Single DES encryption/decryption
  - `DES3`: Triple DES with 3 keys (K1-K2-K1 for 2-key mode)
  - `DES3CBC`: Triple DES in CBC mode
  
- **ETL**: Embedded Template Library
  - Fixed-size containers: `etl::vector`, `etl::array`
  - Error handling: `etl::expected`

## Security Notes

1. **Default Keys**: The factory default key (all zeros) should be changed in production
2. **Key Storage**: Keys should be stored securely, not hardcoded
3. **Random Generation**: Uses `std::random_device` and `std::mt19937` for RndA generation
4. **Parity Bits**: DES/3DES session keys have LSB cleared for parity
5. **IV Management**: ISO mode requires careful IV tracking across operations

## Error Handling

The authentication command can fail with:
- `DesfireError::InvalidResponse`: Unexpected response format
- `DesfireError::AuthenticationFailed`: RndA verification failed
- `DesfireError::InvalidState`: State machine error
- `DesfireError::AuthenticationError`: Card rejected authentication (0xAE)

## Testing

To test authentication:

1. Use a DESFire EV1/EV2 card with factory default keys
2. Select master application (AID 0x000000)
3. Authenticate with key 0 using ISO mode
4. Verify authentication succeeds and session key is generated
5. Test further encrypted operations

## References

- DESFire documentation (see `Docs/Desfire-Authentication.md`)
- DESFire DES authentication flow (see `Docs/Desfire-Des-Authentication.md`)
- Legacy authentication restrictions (see `Docs/DesfireLegacyAuthenticateRestrictions.md`)
