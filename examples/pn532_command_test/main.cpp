/**
 * @file main.cpp
 * @brief PN532 Command Test Application
 * @details Interactive test application for PN532 commands:
 *          - GetFirmwareVersion
 *          - PerformSelfTest
 *          - InListPassiveTarget
 *          - InDataExchange
 */

#include <iostream>
#include <thread>
#include <chrono>

#include "Pn532/Pn532Driver.h"
#include "Pn532/Commands/GetFirmwareVersion.h"
#include "Pn532/Commands/PerformSelfTest.h"
#include "Pn532/Commands/InListPassiveTarget.h"
#include "Pn532/Commands/InDataExchange.h"
#include "Pn532/Pn532ApduAdapter.h"
#include "Comms/Serial/SerialBusWin.hpp"
#include "Utils/Logging.h"
#include "etl/to_string.h"

using namespace pn532;
using namespace error;

// ANSI colors for better output
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

#include "etl/string.h"

// Deduce size for string literals
template <size_t N>
constexpr auto to_etl_string(const char (&literal)[N]) {
    etl::string<N - 1> s{literal};
    return s;
}

void printHeader(const char* title)
{
    std::cout << "\n" << COLOR_BOLD << COLOR_CYAN 
              << "========================================\n"
              << "  " << title << "\n"
              << "========================================" 
              << COLOR_RESET << "\n\n";
}

void printSuccess(const char* msg)
{
    std::cout << COLOR_GREEN << "+ " << msg << COLOR_RESET << "\n";
}

void printError(const char* msg)
{
    std::cout << COLOR_RED << "- " << msg << COLOR_RESET << "\n";
}

void printInfo(const char* msg)
{
    std::cout << COLOR_CYAN << "I " << msg << COLOR_RESET << "\n";
}

void testGetFirmwareVersion(Pn532Driver& driver)
{
    printHeader("Test 1: Get Firmware Version");
    
    GetFirmwareVersion cmd;
    auto result = driver.executeCommand(cmd);
    
    if (result.has_value())
    {
        printSuccess("Firmware version retrieved successfully!");
        
        const auto& info = cmd.getFirmwareInfo();
        std::cout << "  IC:      0x" << std::hex << (int)info.ic << std::dec << "\n";
        std::cout << "  Version: 0x" << std::hex << (int)info.ver << std::dec << "\n";
        std::cout << "  Revision:0x" << std::hex << (int)info.rev << std::dec << "\n";
        std::cout << "  Support: 0x" << std::hex << (int)info.support << std::dec << "\n";
        
        // Decode support flags
        std::cout << "\nSupported features:\n";
        if (info.support & 0x01) std::cout << "  • ISO/IEC 14443 Type A\n";
        if (info.support & 0x02) std::cout << "  • ISO/IEC 14443 Type B\n";
        if (info.support & 0x04) std::cout << "  • ISO 18092\n";
    }
    else
    {
        // get Error information
        auto error = result.error();
        auto errorMessage = etl::string<128>("Failed to get firmware version: ");
        errorMessage += error.toString();
        
        printError(errorMessage.c_str());
    }
}

void testPerformSelfTest(Pn532Driver& driver)
{
    // printHeader("Test 2: Perform Self Test");
    
    // // Test 1: Communication Line Test
    // std::cout << COLOR_YELLOW << "Running Communication Line Test..." << COLOR_RESET << "\n";
    
    // SelfTestOptions opts;
    // opts.test = TestType::CommunicationLine;
    // opts.parameters.push_back(0xDE);
    // opts.parameters.push_back(0xAD);
    // opts.parameters.push_back(0xBE);
    // opts.parameters.push_back(0xEF);
    // opts.verifyEcho = true;
    
    // PerformSelfTest cmd(opts);
    // auto result = driver.executeCommand(cmd);
    
    // if (result.has_value())
    // {
    //     printSuccess("Communication line test passed! (Echo verified)");
    // }
    // else
    // {
    //     printError("Communication line test failed");
    // }
    
    // // Test 2: ROM Checksum Test
    // std::cout << "\n" << COLOR_YELLOW << "Running ROM Checksum Test..." << COLOR_RESET << "\n";
    
    // SelfTestOptions romOpts;
    // romOpts.test = TestType::RomChecksum;
    // romOpts.responseTimeoutMs = 5000; // ROM checksum can take longer
    
    // PerformSelfTest romCmd(romOpts);
    // auto romResult = driver.executeCommand(romCmd);
    auto selfTestResult = driver.performSelftest();
    if (selfTestResult.has_value())
    {
        printSuccess("Self-test passed!");
    }
    else
    {
        printError("Self-test failed");
    }
}

void testGetGeneralStatus(Pn532Driver& driver)
{
    printHeader("Test 2: Get General Status");
    
    auto result = driver.getGeneralStatus();
    
    if (result.has_value())
    {
        printSuccess("General status retrieved successfully!");
        
        const auto& status = result.value();
        std::cout << status.toString()  << "\n";
    }
    else
    {
        printError("Failed to get general status");
    }
}

void testCardDetection(Pn532Driver& driver)
{
    printHeader("Test 3: Card Detection");
    
    printInfo("Place a card near the reader...");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    InListPassiveTargetOptions opts;
    opts.maxTargets = 2;
    opts.target = CardTargetType::TypeA_106kbps;
    
    InListPassiveTarget cmd(opts);
    auto result = driver.executeCommand(cmd);
    
    if (result.has_value())
    {
        const auto& targets = cmd.getDetectedTargets();
        
        if (targets.empty())
        {
            printInfo("No cards detected");
        }
        else
        {
            printSuccess("Card(s) detected!");
            std::cout << "  Number of cards: " << targets.size() << "\n\n";
            
            for (size_t i = 0; i < targets.size(); ++i)
            {
                const auto& target = targets[i];
                
                std::cout << COLOR_BOLD << "  Card #" << (i + 1) << ":" << COLOR_RESET << "\n";
                
                // UID
                std::cout << "    UID:  ";
                for (size_t j = 0; j < target.uid.size(); ++j)
                {
                    std::printf("%02X ", target.uid[j]);
                }
                std::cout << "(" << target.uid.size() << " bytes)\n";
                
                // ATQA
                std::cout << "    ATQA: 0x" << std::hex << target.atqa << std::dec << "\n";
                
                // SAK
                std::cout << "    SAK:  0x" << std::hex << (int)target.sak << std::dec;
                
                // Decode card type from SAK
                std::cout << " (";
                if (target.sak & 0x20) 
                {
                    std::cout << "ISO-DEP/ISO 14443-4";
                }
                else if (target.sak == 0x08) 
                {
                    std::cout << "MIFARE Classic 1K";
                }
                else if (target.sak == 0x09) 
                {
                    std::cout << "MIFARE Mini";
                }
                else if (target.sak == 0x18) 
                {
                    std::cout << "MIFARE Classic 4K";
                }
                else if (target.sak == 0x00) 
                {
                    std::cout << "MIFARE Ultralight";
                }
                else 
                {
                    std::cout << "Unknown";
                }
                std::cout << ")\n";
                
                // ATS (if available)
                if (!target.ats.empty())
                {
                    std::cout << "    ATS:  ";
                    for (size_t j = 0; j < target.ats.size(); ++j)
                    {
                        std::printf("%02X ", target.ats[j]);
                    }
                    std::cout << "\n";
                }
                
                std::cout << "\n";
            }
        }
    }
    else
    {
        printError("Card detection failed");
    }
}

void testDataExchange(Pn532Driver& driver)
{
    printHeader("Test 4: Data Exchange (APDU)");
    
    printInfo("This test will attempt to communicate with a detected card");
    printInfo("Detecting card first...");
    
    // First detect a card
    InListPassiveTargetOptions detectOpts;
    detectOpts.maxTargets = 1;
    detectOpts.target = CardTargetType::TypeA_106kbps;
    
    InListPassiveTarget detectCmd(detectOpts);
    auto detectResult = driver.executeCommand(detectCmd);
    
    if (!detectResult.has_value() || detectCmd.getDetectedTargets().empty())
    {
        printError("No card detected. Cannot perform data exchange.");
        return;
    }
    
    printSuccess("Card detected! Attempting data exchange...");
    
    // Send a simple APDU command (e.g., SELECT application)
    // This is a generic ISO 7816-4 SELECT command
    InDataExchangeOptions opts;
    opts.targetNumber = 0x01;
    
    // Example: SELECT MF (Master File) - CLA=00 INS=A4 P1=00 P2=00 Lc=00
    opts.payload.push_back(0x00);  // CLA
    opts.payload.push_back(0xA4);  // INS (SELECT)
    opts.payload.push_back(0x00);  // P1
    opts.payload.push_back(0x00);  // P2
    opts.payload.push_back(0x00);  // Lc (no data)
    
    opts.responseTimeoutMs = 2000;
    
    InDataExchange cmd(opts);
    auto result = driver.executeCommand(cmd);
    
    if (result.has_value())
    {
        std::cout << "\n  Status Code: 0x" << std::hex << (int)cmd.getStatusByte() << std::dec << "\n";
        std::cout << "  Status:      " << cmd.getStatusString() << "\n";
        
        if (cmd.isSuccess())
        {
            printSuccess("Data exchange successful!");
            
            const auto& responseData = cmd.getResponseData();
            if (!responseData.empty())
            {
                std::cout << "\n  Response Data (" << responseData.size() << " bytes): ";
                for (size_t i = 0; i < responseData.size(); ++i)
                {
                    std::printf("%02X ", responseData[i]);
                }
                std::cout << "\n";
            }
            else
            {
                printInfo("No response data (command accepted)");
            }
        }
        else
        {
            printError("Card returned error status");
            std::cout << "  This is normal if the card doesn't support this command\n";
        }
    }
    else
    {
        printError("Data exchange failed");
    }
}

int main(int argc, char* argv[])
{
    std::cout << COLOR_BOLD << COLOR_GREEN 
              << "\n=========================================\n"
              << "|     PN532 Command Test Application     |\n"
              << "=========================================\n" 
              << COLOR_RESET << "\n";
    
    // Get COM port from command line or use default
    etl::string<256> comPort = "COM3";
    if (argc > 1)
    {
        comPort = etl::string<256>(argv[1]);
    }
    
    std::cout << "Using COM port: " << COLOR_CYAN << comPort.c_str() << COLOR_RESET << "\n";
    std::cout << "Baudrate:       " << COLOR_CYAN << "115200" << COLOR_RESET << "\n\n";
    
    // Create serial bus
    comms::serial::SerialBusWin serialBus(comPort, 115200);
    
    // Open connection
    printInfo("Opening serial connection...");
    auto openResult = serialBus.open();
    if (!openResult.has_value())
    {
        auto error = openResult.error();
        auto errorMessage = etl::string<128>("Failed to open serial port: ");
        errorMessage += error.toString();
        
        printError(errorMessage.c_str());
        return 1;
    }
    
    printSuccess("Serial port opened successfully!");
    
    // Create PN532 driver
    Pn532Driver driver(serialBus);
    Pn532ApduAdapter apduAdapter(driver);

    
    printInfo("Initializing PN532 driver...");
    driver.init();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    apduAdapter.detectCard(); // Pre-initialize card detection

    
    // Run tests
    try
    {
        std::cout << "\nPress ENTER to continue to get Firmware Version...";
        std::cin.get();

        testGetFirmwareVersion(driver);
        
        std::cout << "\nPress ENTER to continue to self-test...";
        std::cin.get();
        
        testPerformSelfTest(driver);
        
        // std::cout << "\nPress ENTER to continue to card detection...";
        // std::cin.get();
        
        // testCardDetection(driver);

        std::cout << "\nPress ENTER to continue to general status...";
        std::cin.get();

        testGetGeneralStatus(driver);
        
        std::cout << "\nPress ENTER to continue to data exchange...";
        std::cin.get();
        
        testDataExchange(driver);
        
        // Summary
        std::cout << "\n\n" << COLOR_BOLD << COLOR_GREEN 
                  << "╔════════════════════════════════════════╗\n"
                  << "║  All Tests Completed!                  ║\n"
                  << "╚════════════════════════════════════════╝\n" 
                  << COLOR_RESET << "\n";
    }
    catch (const std::exception& e)
    {
        printError("Exception occurred!");
        std::cout << "  " << e.what() << "\n";
        return 1;
    }
    
    // Close connection
    serialBus.close();
    printInfo("Serial port closed.");
    
    return 0;
}
