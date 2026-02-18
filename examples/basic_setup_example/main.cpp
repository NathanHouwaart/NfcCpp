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
#include "Nfc/Desfire/DesfireAuthMode.h"
#include "Utils/Logging.h"
#include "Utils/Timing.h"

using namespace pn532;
using namespace nfc;
using namespace comms;
using namespace comms::serial;
using namespace error;

int main()
{
    // Hardware setup
    SerialBusWin serial("COM5", 115200);
    Pn532Driver pn532(serial);
    Pn532ApduAdapter adapter(pn532); // Implements both interfaces

    // Create reader context
    CardManager cardManager(
        adapter, // IApduTransceiver
        adapter, // ICardDetector (same object!)
        ReaderCapabilities::pn532()
    );
    cardManager.setWire(WireKind::Iso); // Use ISO wire for DESFire ISO mode

    serial.init();
    pn532.init();
    pn532.setSamConfiguration(0x01); // Normal mode, no timeout, use IRQ
    pn532.setMaxRetries(1); // Infinite retries for card detection

    // Detect card
    auto cardInfo    = cardManager.detectCard();
    auto cardSession = cardManager.createSession();

    if (!cardSession.has_value())
    {
        LOG_ERROR("Failed to create session");
        std::cout << "- Session creation failed" << std::endl;
        return -1;
    }else{
        std::cout << "+ Session created successfully" << std::endl;
    }
    
    CardSession* session = cardSession.value();
    const CardInfo& card = session->getCardInfo();
    
    // Demonstrate type-specific access
    std::cout << "\nAccessing card based on detected type..." << std::endl;

    auto* desfire = session->getCardAs<DesfireCard>();

    // Guard: if no DESFire object, bail out early
    if (!desfire) {
        std::cout << " Other card type or DESFire not initialized in session" << std::endl;
        return 0;
    }

    std::cout << "  - DESFire card object available" << std::endl;
    std::cout << "  - Ready for DESFire operations (selectApplication, authenticate, etc.)" << std::endl;

    // Demonstrate DESFire authentication
    std::cout << "\nAttempting DESFire authentication..." << std::endl;

    // Select master application (AID 0x000000)
    etl::array<uint8_t, 3> masterAid = {0x00, 0x00, 0x00};
    auto selectResult = desfire->selectApplication(masterAid);
    if (!selectResult) {
        std::cout << "  - Failed to select master application: " << selectResult.error().toString().c_str() << std::endl;
        return 0;
    }

    std::cout << "  + Master application selected" << std::endl;

    // Prepare factory default key (all zeros for 2K3DES)
    etl::vector<uint8_t, 24> defaultKey;
    for (int i = 0; i < 16; ++i) {
        defaultKey.push_back(0x00);
    }

    // Authenticate with key 0 using ISO mode (0x1A)
    auto authResult = desfire->authenticate(0, defaultKey, DesfireAuthMode::ISO);
    if (!authResult) {
        std::cout << "  - Authentication failed: " << authResult.error().toString().c_str() << std::endl;
    } else {
        std::cout << "  + Authentication successful!" << std::endl;
        std::cout << "  + Session key established" << std::endl;
        std::cout << "  + Secure channel active" << std::endl;
    }
    
    return 0;
}