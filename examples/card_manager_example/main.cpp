/**
 * @file main.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief CardManager example - Demonstrates card detection and session management
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <iostream>
#include <cstdlib>
#include <limits>
#include "Comms/Serial/SerialBusWin.hpp"
#include "Pn532/Pn532Driver.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Nfc/Card/CardManager.h"
#include "Nfc/Card/ReaderCapabilities.h"
#include "Utils/Logging.h"
#include "Utils/Timing.h"

using namespace pn532;
using namespace nfc;
using namespace comms;
using namespace comms::serial;
using namespace error;

void printSeparator(const char* title = nullptr)
{
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    if (title)
    {
        std::cout << "  " << title << std::endl;
        std::cout << "========================================" << std::endl;
    }
}

void waitForEnter(const char* message = "Press ENTER to continue...")
{
    std::cout << "\n" << message << std::endl;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
}

bool initializeReader(Pn532Driver& driver)
{
    LOG_INFO("Initializing PN532...");
    
    // Configure SAM (required for NFC operations)
    auto samResult = driver.setSamConfiguration(0x01);
    if (!samResult.has_value())
    {
        LOG_ERROR("Failed to configure SAM");
        return false;
    }
    
    // Set retry count
    auto retryResult = driver.setMaxRetries(3);
    if (!retryResult.has_value())
    {
        LOG_ERROR("Failed to set max retries");
        return false;
    }
    
    LOG_INFO("PN532 initialized successfully");
    return true;
}

void demonstrateCardDetection(CardManager& cardManager)
{
    printSeparator("Card Detection");
    
    std::cout << "Please place a card on the reader..." << std::endl;
    
    auto cardResult = cardManager.detectCard();
    
    if (!cardResult.has_value())
    {
        LOG_ERROR("Card detection failed");
        std::cout << "- No card detected or error occurred" << std::endl;
        return;
    }
    
    CardInfo card = cardResult.value();
    
    std::cout << "\n+ Card detected!" << std::endl;
    std::cout << card.toString().c_str() << std::endl;
}

void demonstrateSessionManagement(CardManager& cardManager)
{
    printSeparator("Session Management");
    
    // Create session from the already-detected card
    std::cout << "Creating card session from detected card..." << std::endl;
    auto sessionResult = cardManager.createSession();
    
    if (!sessionResult.has_value())
    {
        LOG_ERROR("Failed to create session");
        std::cout << "- Session creation failed" << std::endl;
        return;
    }
    
    CardSession* session = sessionResult.value();
    std::cout << "+ Session created successfully" << std::endl;
    
    // Get the card info from the session
    const CardInfo& card = session->getCardInfo();
    
    // Demonstrate type-specific access
    std::cout << "\nAccessing card based on detected type..." << std::endl;
    
    switch (card.type)
    {
        case CardType::MifareDesfire:
        {
            std::cout << "+ Card type: MIFARE DESFire" << std::endl;
            auto* desfire = session->getCardAs<DesfireCard>();
            if (desfire)
            {
                std::cout << "  - DESFire card object available" << std::endl;
                std::cout << "  - Ready for DESFire operations (selectApplication, authenticate, etc.)" << std::endl;
            }
            else
            {
                std::cout << "  - DESFire card not yet initialized in session" << std::endl;
            }
            break;
        }
        
        case CardType::MifareClassic:
        {
            std::cout << "+ Card type: MIFARE Classic" << std::endl;
            auto* classic = session->getCardAs<MifareClassicCard>();
            if (classic)
            {
                std::cout << "  - MIFARE Classic card object available" << std::endl;
                std::cout << "  - Ready for Classic operations (authenticate, read/write blocks)" << std::endl;
            }
            else
            {
                std::cout << "  - MIFARE Classic card not yet initialized in session" << std::endl;
            }
            break;
        }
        
        case CardType::MifareUltralight:
        {
            std::cout << "+ Card type: MIFARE Ultralight" << std::endl;
            auto* ultralight = session->getCardAs<UltralightCard>();
            if (ultralight)
            {
                std::cout << "  - Ultralight card object available" << std::endl;
                std::cout << "  - Ready for Ultralight operations (read/write pages)" << std::endl;
            }
            else
            {
                std::cout << "  - Ultralight card not yet initialized in session" << std::endl;
            }
            break;
        }
        
        case CardType::Ntag213_215_216:
        {
            std::cout << "+ Card type: NTAG213/215/216" << std::endl;
            auto* ntag = session->getCardAs<UltralightCard>();
            if (ntag)
            {
                std::cout << "  - NTAG card object available" << std::endl;
                std::cout << "  - Ready for NTAG operations (read/write pages, NDEF)" << std::endl;
            }
            else
            {
                std::cout << "  - NTAG card not yet initialized in session" << std::endl;
            }
            break;
        }
        
        case CardType::ISO14443_4_Generic:
        {
            std::cout << "+ Card type: ISO14443-4 Generic" << std::endl;
            std::cout << "  - Generic ISO14443-4 compliant card" << std::endl;
            std::cout << "  - Can use APDU commands directly" << std::endl;
            break;
        }
        
        default:
            std::cout << "! Card type: Unknown or unsupported" << std::endl;
            break;
    }
}

void demonstrateCardPresence(CardManager& cardManager)
{
    printSeparator("Card Presence Monitoring");
    
    std::cout << "Monitoring card presence for 10 seconds..." << std::endl;
    std::cout << "(Remove the card to see the detection)" << std::endl;
    
    bool wasPresent = true;
    uint32_t startTime = utils::get_tick_ms();
    
    while (utils::get_tick_ms() - startTime < 10000) // 10 seconds
    {
        bool isPresent = cardManager.isCardPresent();
        
        if (isPresent != wasPresent)
        {
            if (isPresent)
            {
                std::cout << "\n+ Card detected in field" << std::endl;
            }
            else
            {
                std::cout << "\n- Card removed from field" << std::endl;
            }
            wasPresent = isPresent;
        }
        
        utils::delay_ms(100); // Check every 100ms
    }
    
    std::cout << "\nMonitoring complete." << std::endl;
}

void demonstrateMultipleDetections(CardManager& cardManager)
{
    printSeparator("Multiple Card Detections");
    
    std::cout << "This demonstrates detecting different cards in sequence." << std::endl;
    std::cout << "Place different cards on the reader and press ENTER each time." << std::endl;
    
    for (int i = 1; i <= 3; ++i)
    {
        std::cout << "\n--- Detection " << i << " ---" << std::endl;
        waitForEnter("Place a card and press ENTER...");
        
        // Clear previous session
        cardManager.clearSession();
        
        auto cardResult = cardManager.detectCard();
        
        if (cardResult.has_value())
        {
            CardInfo card = cardResult.value();
            std::cout << "+ Card " << i << " detected:" << std::endl;
            std::cout << "  UID: ";
            for (size_t j = 0; j < card.uid.size(); ++j)
            {
                std::cout << std::hex << std::uppercase 
                         << (int)card.uid[j] 
                         << (j < card.uid.size() - 1 ? " " : "");
            }
            std::cout << std::dec << std::endl;
            
            const char* typeStr = "Unknown";
            switch (card.type)
            {
                case CardType::MifareDesfire: typeStr = "DESFire"; break;
                case CardType::MifareClassic: typeStr = "Classic"; break;
                case CardType::MifareUltralight: typeStr = "Ultralight"; break;
                case CardType::Ntag213_215_216: typeStr = "NTAG"; break;
                case CardType::ISO14443_4_Generic: typeStr = "ISO14443-4"; break;
                default: typeStr = "Unknown"; break;
            }
            std::cout << "  Type: " << typeStr << std::endl;
        }
        else
        {
            std::cout << "- No card detected" << std::endl;
        }
    }
    
    cardManager.clearSession();
}

int main(int argc, char* argv[])
{
    std::cout << "\n";
    std::cout << "=========================================" << std::endl;
    std::cout << "|  CardManager Example Application       |" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Parse command line arguments
    if (argc < 2)
    {
        std::cerr << "\nUsage: " << argv[0] << " <COM_PORT>" << std::endl;
        std::cerr << "Example: " << argv[0] << " COM5" << std::endl;
        return 1;
    }
    
    const char* comPort = argv[1];
    const int baudRate = 115200;
    
    std::cout << "\nUsing COM port: " << comPort << std::endl;
    std::cout << "Baudrate:       " << baudRate << std::endl;
    
    // Initialize serial bus
    printSeparator();
    std::cout << "I Opening serial connection..." << std::endl;
    
    etl::string<256> portName(comPort);
    SerialBusWin serialBus(portName, baudRate);
    
    auto initResult = serialBus.init();
    if (!initResult.has_value())
    {
        std::cerr << "- Failed to initialize serial port" << std::endl;
        return 1;
    }
    
    std::cout << "+ Serial port opened successfully!" << std::endl;
    
    // Initialize PN532 driver
    Pn532Driver pn532Driver(serialBus);
    
    if (!initializeReader(pn532Driver))
    {
        std::cerr << "- Failed to initialize PN532" << std::endl;
        return 1;
    }
    
    // Create APDU adapter (implements both IApduTransceiver and ICardDetector)
    Pn532ApduAdapter apduAdapter(pn532Driver);
    
    // Setup reader capabilities (use PN532 defaults)
    ReaderCapabilities capabilities = ReaderCapabilities::pn532();
    
    // Create CardManager
    CardManager cardManager(apduAdapter, apduAdapter, capabilities);
    
    std::cout << "\n+ CardManager created successfully" << std::endl;
    std::cout << "  Max APDU size: " << capabilities.maxApduSize << " bytes" << std::endl;
    
    // Run demonstrations
    try
    {
        waitForEnter("\nPress ENTER to start demonstration...");
        
        // Demo 1: Basic card detection
        demonstrateCardDetection(cardManager);
        waitForEnter();
        
        // Demo 2: Session management and type-specific access
        demonstrateSessionManagement(cardManager);
        waitForEnter();
        
        // Demo 3: Card presence monitoring
        demonstrateCardPresence(cardManager);
        waitForEnter();
        
        // Demo 4: Multiple card detections
        demonstrateMultipleDetections(cardManager);
        
        printSeparator();
        std::cout << "\n+ All demonstrations completed successfully!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n! Exception: " << e.what() << std::endl;
        return 1;
    }
    
    // Cleanup
    cardManager.clearSession();
    
    printSeparator();
    std::cout << "\nThank you for using CardManager Example!" << std::endl;
    std::cout << "\n";
    
    return 0;
}
