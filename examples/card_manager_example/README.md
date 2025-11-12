# CardManager Example

This example demonstrates the high-level CardManager API for card detection, session management, and type-specific card operations.

## Overview

The CardManager provides a simplified interface for working with NFC cards:

1. **Card Detection** - Automatically detects cards and identifies their type
2. **Session Management** - Creates type-specific card sessions
3. **Type-Safe Access** - Access card-specific operations based on detected type
4. **Presence Monitoring** - Monitor when cards enter/leave the reader field

## Features Demonstrated

### 1. Basic Card Detection
- Detects cards placed on the reader
- Displays card information (UID, ATQA, SAK, type)
- Automatically identifies card type (DESFire, Classic, Ultralight, NTAG, etc.)

### 2. Session Management
- Creates CardSession with type-specific card objects
- Demonstrates type-safe access using `getCardAs<T>()`
- Shows how to access operations for each card type

### 3. Card Presence Monitoring
- Real-time monitoring of card presence
- Detects when cards are removed from the field
- Useful for implementing card removal detection

### 4. Multiple Card Detections
- Demonstrates detecting different cards in sequence
- Shows session cleanup between detections
- Useful for applications handling multiple cards

## Building

The example is built automatically as part of the main project:

```bash
cd build
cmake ..
cmake --build .
```

The executable will be located at: `build/examples/Debug/card_manager_example.exe`

## Usage

```bash
.\build\examples\Debug\card_manager_example.exe COM5
```

Replace `COM5` with your PN532 reader's COM port.

## Code Structure

### Initialization

```cpp
// Create hardware bus
SerialBusWin serialBus;
serialBus.open("COM5", 115200);

// Create PN532 driver
Pn532Driver pn532Driver(serialBus);

// Create APDU adapter (implements IApduTransceiver and ICardDetector)
Pn532ApduAdapter apduAdapter(pn532Driver);

// Define reader capabilities
ReaderCapabilities capabilities;
capabilities.maxApduSize = 256;
capabilities.supportsIso14443A = true;
capabilities.supportsDESFire = true;
// ... etc

// Create CardManager
CardManager cardManager(apduAdapter, apduAdapter, capabilities);
```

### Card Detection

```cpp
// Detect card
auto cardResult = cardManager.detectCard();
if (cardResult.has_value())
{
    CardInfo card = cardResult.value();
    std::cout << card.toString() << std::endl;
    
    // Card type is automatically detected
    if (card.type == CardType::MifareDesfire)
    {
        // This is a DESFire card
    }
}
```

### Session Management

```cpp
// Create session (uses previously detected card)
auto sessionResult = cardManager.createSession();
if (sessionResult.has_value())
{
    CardSession* session = sessionResult.value();
    
    // Type-safe access to card-specific operations
    if (auto* desfire = session->getCardAs<DesfireCard>())
    {
        // Use DESFire-specific operations
        // desfire->selectApplication(...)
        // desfire->authenticate(...)
    }
    else if (auto* classic = session->getCardAs<MifareClassicCard>())
    {
        // Use Classic-specific operations
        // classic->authenticate(...)
        // classic->readBlock(...)
    }
}
```

### Card Presence Checking

```cpp
// Check if card is still present
bool isPresent = cardManager.isCardPresent();
if (!isPresent)
{
    std::cout << "Card removed from field" << std::endl;
    cardManager.clearSession();
}
```

## Supported Card Types

The CardManager automatically detects and handles:

- **MIFARE DESFire** - Advanced smart card with file system
- **MIFARE Classic** - 1K/4K cards with block-based storage
- **MIFARE Ultralight** - Low-cost read/write tags
- **NTAG213/215/216** - NFC Forum Type 2 tags
- **Generic ISO14443-4** - Any ISO14443-4 compliant card

## Expected Output

```
=========================================
|  CardManager Example Application       |
=========================================

Using COM port: COM5
Baudrate:       115200

========================================
I Opening serial connection...
+ Serial port opened successfully!
Initializing PN532...
+ PN532 initialized successfully

+ CardManager created successfully
  Max APDU size: 256 bytes

Press ENTER to start demonstration...

========================================
  Card Detection
========================================
Please place a card on the reader...

+ Card detected!
Card Type: MIFARE DESFire
UID (7 bytes): 04 1A 56 1A 99 5B 80
ATQA: 03 44
SAK: 20
ATS: 06 75 77 81 02 80

Press ENTER to continue...

========================================
  Session Management
========================================
Creating card session...
+ Session created successfully

Accessing card based on detected type...
+ Card type: MIFARE DESFire
  - DESFire card object available
  - Ready for DESFire operations (selectApplication, authenticate, etc.)

...
```

## API Usage Pattern

The typical workflow with CardManager:

1. **Initialize** - Create CardManager with APDU adapter and capabilities
2. **Detect** - Call `detectCard()` to find and identify a card
3. **Create Session** - Call `createSession()` to get type-specific card object
4. **Use Card** - Access card-specific operations via `getCardAs<T>()`
5. **Monitor** - Optionally use `isCardPresent()` to detect removal
6. **Cleanup** - Call `clearSession()` when done

## Learn More

- See `Include/Nfc/Card/CardManager.h` for the full API
- See `Include/Nfc/Card/CardSession.h` for session management
- See `Include/Nfc/Card/CardInfo.h` for card type detection logic
- See `Include/Nfc/Cards/` for card-specific operation APIs
