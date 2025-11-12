# PN532 Command Test Application

An interactive test application for the PN532 NFC reader that demonstrates all four core PN532 commands.

## Features

This application tests the following PN532 commands:
1. **GetFirmwareVersion** - Reads and displays firmware information
2. **PerformSelfTest** - Runs communication line and ROM checksum tests
3. **InListPassiveTarget** - Detects NFC cards and displays their information
4. **InDataExchange** - Exchanges APDU commands with detected cards

## Building

The application is built as part of the main NfcCpp project:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --target pn532_command_test
```

The executable will be created in `build/examples/pn532_command_test.exe`

## Usage

```powershell
# Using default COM port (COM3)
./build/examples/pn532_command_test.exe

# Specifying a COM port
./build/examples/pn532_command_test.exe COM4
```

## Interactive Flow

The application runs through tests interactively:

1. **Firmware Version Test**
   - Reads IC, Version, Revision, and Support flags
   - Decodes supported features (ISO14443 Type A/B, ISO18092)

2. **Self Test**
   - Runs Communication Line test with echo verification
   - Runs ROM Checksum test

3. **Card Detection**
   - Detects ISO14443 Type A cards
   - Displays UID, ATQA, SAK, and ATS (if present)
   - Identifies card type (Mifare Classic, Ultralight, DESFire)

4. **Data Exchange**
   - Sends a SELECT APDU command (00 A4 04 00 00)
   - Displays the card's response
   - Shows status code and description

Press ENTER between tests to continue.

## Hardware Requirements

- PN532 NFC Reader module
- USB-to-Serial adapter (or built-in serial on PN532)
- Windows PC with available COM port

## Configuration

The PN532 module should be configured for:
- **Baudrate**: 115200
- **Interface**: UART/HSU (High Speed UART)

## Output

The application uses colored ANSI terminal output:
- 🟢 Green checkmarks (✓) for success
- 🔴 Red X marks (✗) for errors
- 🟡 Yellow info markers (ℹ) for informational messages
- 🔵 Cyan for important values

## Example Output

```
╔════════════════════════════════════════╗
║  PN532 Command Test Application       ║
╚════════════════════════════════════════╝

Using COM port: COM3
Baudrate:       115200

ℹ Opening serial connection...
✓ Serial port opened successfully!

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  Test 1: Get Firmware Version
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

ℹ Requesting firmware version...
✓ Firmware version retrieved successfully!
  IC:      0x32
  Version: 0x01
  Revision:0x06
  Support: 0x07

Supported features:
  • ISO/IEC 14443 Type A
  • ISO/IEC 14443 Type B
  • ISO 18092

Press ENTER to continue to next test...
```

## Troubleshooting

**Failed to open serial port**
- Check COM port number with Device Manager
- Ensure no other application is using the port
- Verify PN532 is properly connected

**No cards detected**
- Ensure PN532 antenna is properly connected
- Card must be within ~4cm of reader
- Try different cards (Mifare, Ultralight, DESFire)

**Data exchange failed**
- Card may not support ISO-DEP protocol
- Some cards require authentication before APDU exchange
- Check SAK byte - bit 5 should be set for ISO-DEP

## Notes

- The application is designed for interactive testing, not automated testing
- Each test can be run independently (press ENTER to skip)
- All responses are displayed in hexadecimal format
- The SELECT APDU (00 A4 04 00 00) is a standard ISO7816 command
