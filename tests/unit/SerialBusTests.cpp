#include <gtest/gtest.h>
#include "Comms/Serial/SerialBusWin.hpp"
#include "Error/Error.h"

using namespace comms::serial;
using namespace error;

// Test Fixture for Serial Bus
class SerialBusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code before each test
    }

    void TearDown() override {
        // Cleanup code after each test
    }
};

// Test: Serial Bus Construction
TEST_F(SerialBusTest, Construction) {
    SerialBusWin serialBus("COM1", 9600);
    EXPECT_FALSE(serialBus.isOpen());
}

// Test: Serial Bus Baud Rate
TEST_F(SerialBusTest, BaudRate) {
    SerialBusWin serialBus("COM3", 115200);
    // Serial bus created successfully
    EXPECT_FALSE(serialBus.isOpen());
}

// Test: Invalid Port Opening (will fail if port doesn't exist)
TEST_F(SerialBusTest, OpenInvalidPort) {
    SerialBusWin serialBus("COM999", 9600);
    auto result = serialBus.open();
    
    // Should fail on non-existent port
    EXPECT_FALSE(result.has_value());
    if (!result) {
        EXPECT_TRUE(result.error().is<HardwareError>());
    }
}

// Test: Baud Rate Setting (without actual port)
TEST_F(SerialBusTest, BaudRateProperty) {
    SerialBusWin serialBus("COM1", 9600);
    
    // This will fail if port is not open, which is expected
    auto result = serialBus.setBaudRate(115200);
    
    // We expect this to fail since port isn't open
    EXPECT_FALSE(result.has_value());
}

// Test: Multiple Close Calls (should be safe)
TEST_F(SerialBusTest, MultipleCloseSafe) {
    SerialBusWin serialBus("COM1", 9600);
    
    // Close without opening should be safe
    EXPECT_NO_THROW(serialBus.close());
    EXPECT_NO_THROW(serialBus.close());
}

// Mock Test: Write without opening
TEST_F(SerialBusTest, WriteWithoutOpen) {
    SerialBusWin serialBus("COM1", 9600);
    
    etl::vector<uint8_t, 10> data{0x01, 0x02, 0x03};
    auto result = serialBus.write(data);
    
    // Should fail - port not open
    EXPECT_FALSE(result.has_value());
}

// Mock Test: Read without opening
TEST_F(SerialBusTest, ReadWithoutOpen) {
    SerialBusWin serialBus("COM1", 9600);
    
    etl::vector<uint8_t, 100> buffer;
    auto result = serialBus.read(buffer, 10);
    
    // Should fail - port not open
    EXPECT_FALSE(result.has_value());
}

// Test: Parity Settings
TEST_F(SerialBusTest, ParitySettings) {
    SerialBusWin serialBus("COM1", 9600);
    
    // These will fail without open port, but test the API
    auto result = serialBus.setParity(Parity::None);
    EXPECT_FALSE(result.has_value());
}

// Test: Available bytes (on closed port)
TEST_F(SerialBusTest, AvailableOnClosedPort) {
    SerialBusWin serialBus("COM1", 9600);
    
    size_t available = serialBus.available();
    EXPECT_EQ(available, 0);
}

// Integration Test: Full initialization sequence (will fail without real hardware)
TEST_F(SerialBusTest, DISABLED_FullInitSequence) {
    // This test is disabled by default as it requires real hardware
    // To enable, run with: --gtest_also_run_disabled_tests
    
    SerialBusWin serialBus("COM1", 9600);
    
    auto result = serialBus.init();
    EXPECT_TRUE(result.has_value());
    
    if (result) {
        EXPECT_TRUE(serialBus.isOpen());
        serialBus.close();
        EXPECT_FALSE(serialBus.isOpen());
    }
}

// Test: Property getter/setter
TEST_F(SerialBusTest, PropertyGetSet) {
    SerialBusWin serialBus("COM1", 9600);
    
    // Get property on closed port
    auto baudResult = serialBus.getProperty(comms::BusProperty::BaudRate);
    
    // Should return the configured baud rate even if not open
    EXPECT_TRUE(baudResult.has_value());
    if (baudResult) {
        EXPECT_EQ(*baudResult, 9600);
    }
}
