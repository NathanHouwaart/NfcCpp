#include <iostream>
#include "Pn532/Pn532Driver.h"
#include "Comms/Serial/ISerialBus.hpp"
#include "Error/Error.h"
#include "Utils/Logging.h"

int main() {
    std::cout << "NfcCpp Library - Basic PN532 Example" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    LOG_INFO("Starting PN532 example application");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    // TODO: Add your PN532 initialization and usage code here
    // Example structure:
    // 1. Create a serial bus instance
    // 2. Initialize the PN532 driver
    // 3. Configure the device
    // 4. Start communication
    
    std::cout << "\nThis is a template example." << std::endl;
    std::cout << "Please implement your PN532 usage logic here." << std::endl;
    
    return 0;
}
