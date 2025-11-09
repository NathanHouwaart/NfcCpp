#include <gtest/gtest.h>
#include "Comms/Serial/SerialBusWin.hpp"

using namespace comms::serial;

// NOTE: These tests require actual hardware connected
// Configure the correct COM port for your system

class SerialIntegrationTest : public ::testing::Test {
protected:
    // Change this to your actual COM port
    const char* testPort = "COM3";
    const uint32_t testBaudRate = 115200;
    
    void SetUp() override {
        // Setup before each test
    }
    
    void TearDown() override {
        // Cleanup after each test
    }
};

// Test: Open and close real serial port
TEST_F(SerialIntegrationTest, DISABLED_OpenClose) {
    SerialBusWin serialBus(testPort, testBaudRate);
    
    auto openResult = serialBus.open();
    EXPECT_TRUE(openResult.has_value()) << "Failed to open serial port. Check if " << testPort << " exists";
    
    if (openResult) {
        EXPECT_TRUE(serialBus.isOpen());
        serialBus.close();
        EXPECT_FALSE(serialBus.isOpen());
    }
}

// Test: Full initialization
TEST_F(SerialIntegrationTest, DISABLED_FullInit) {
    SerialBusWin serialBus(testPort, testBaudRate);
    
    auto initResult = serialBus.init();
    EXPECT_TRUE(initResult.has_value());
    
    if (initResult) {
        EXPECT_TRUE(serialBus.isOpen());
        serialBus.close();
    }
}

// Test: Write data to serial port
TEST_F(SerialIntegrationTest, DISABLED_WriteData) {
    SerialBusWin serialBus(testPort, testBaudRate);
    
    auto initResult = serialBus.init();
    ASSERT_TRUE(initResult.has_value());
    
    etl::vector<uint8_t, 10> testData{0x01, 0x02, 0x03, 0x04, 0x05};
    auto writeResult = serialBus.write(testData);
    
    EXPECT_TRUE(writeResult.has_value());
    
    serialBus.close();
}

// Test: Loopback test (requires TX and RX to be connected)
TEST_F(SerialIntegrationTest, DISABLED_Loopback) {
    SerialBusWin serialBus(testPort, testBaudRate);
    
    auto initResult = serialBus.init();
    ASSERT_TRUE(initResult.has_value());
    
    // Write data
    etl::vector<uint8_t, 10> testData{0xAA, 0xBB, 0xCC};
    auto writeResult = serialBus.write(testData);
    ASSERT_TRUE(writeResult.has_value());
    
    // Small delay for loopback
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Read back
    etl::vector<uint8_t, 100> readBuffer;
    auto readResult = serialBus.read(readBuffer, testData.size());
    
    if (readResult.has_value()) {
        EXPECT_EQ(*readResult, testData.size());
        for (size_t i = 0; i < testData.size(); ++i) {
            EXPECT_EQ(readBuffer[i], testData[i]);
        }
    }
    
    serialBus.close();
}
