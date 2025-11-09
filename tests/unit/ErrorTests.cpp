#include <gtest/gtest.h>
#include "Error/Error.h"
#include "Error/HardwareError.h"
#include "Error/Pn532Error.h"

using namespace error;

// Test: Error creation from different layers
TEST(ErrorTest, CreateFromHardware) {
    Error err = Error::fromHardware(HardwareError::DeviceNotFound);
    
    EXPECT_TRUE(err.is<HardwareError>());
    EXPECT_FALSE(err.is<Pn532Error>());
    
    auto hwErr = err.get<HardwareError>();
    EXPECT_EQ(hwErr, HardwareError::DeviceNotFound);
}

TEST(ErrorTest, CreateFromPn532) {
    Error err = Error::fromPn532(Pn532Error::Timeout);
    
    EXPECT_TRUE(err.is<Pn532Error>());
    EXPECT_FALSE(err.is<HardwareError>());
}

// Test: Error toString method
TEST(ErrorTest, ToStringHardware) {
    Error err = Error::fromHardware(HardwareError::DeviceNotFound);
    auto str = err.toString();
    
    EXPECT_GT(str.size(), 0);
    // Should contain "Hardware" and "Error"
    EXPECT_TRUE(str.find("Hardware") != etl::string<160>::npos || 
                str.find("HARDWARE") != etl::string<160>::npos);
}

TEST(ErrorTest, ToStringPn532) {
    Error err = Error::fromPn532(Pn532Error::Timeout);
    auto str = err.toString();
    
    EXPECT_GT(str.size(), 0);
}

// Test: Layer name retrieval
TEST(ErrorTest, LayerNames) {
    Error hwErr = Error::fromHardware(HardwareError::Unknown);
    Error pn532Err = Error::fromPn532(Pn532Error::Timeout);
    Error linkErr = Error::fromLink(LinkError::Timeout);
    
    // All should produce valid strings
    EXPECT_GT(hwErr.toString().size(), 0);
    EXPECT_GT(pn532Err.toString().size(), 0);
    EXPECT_GT(linkErr.toString().size(), 0);
}

// Test: Error type checking
TEST(ErrorTest, TypeChecking) {
    Error err = Error::fromHardware(HardwareError::NotSupported);
    
    EXPECT_TRUE(err.is<HardwareError>());
    EXPECT_FALSE(err.is<Pn532Error>());
    EXPECT_FALSE(err.is<LinkError>());
}

// Test: Different error values
TEST(ErrorTest, DifferentValues) {
    Error err1 = Error::fromHardware(HardwareError::DeviceNotFound);
    Error err2 = Error::fromHardware(HardwareError::NotSupported);
    
    auto val1 = err1.get<HardwareError>();
    auto val2 = err2.get<HardwareError>();
    
    EXPECT_NE(val1, val2);
}
