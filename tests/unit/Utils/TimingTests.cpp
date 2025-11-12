/**
 * @file TimingTests.cpp
 * @brief Unit tests for cross-platform timing utilities
 */

#include <gtest/gtest.h>
#include "Utils/Timing.h"

// Test basic delay functionality
TEST(TimingTests, DelayMilliseconds)
{
    uint32_t start = utils::get_tick_ms();
    
    utils::delay_ms(100);  // Delay for 100ms
    
    uint32_t elapsed = utils::elapsed_ms(start);
    
    // Allow some tolerance (±20ms) due to OS scheduling
    EXPECT_GE(elapsed, 90u);
    EXPECT_LE(elapsed, 120u);
}

// Test tick counter
TEST(TimingTests, GetTickMs)
{
    uint32_t tick1 = utils::get_tick_ms();
    utils::delay_ms(50);
    uint32_t tick2 = utils::get_tick_ms();
    
    EXPECT_GT(tick2, tick1);
}

// Test elapsed time calculation
TEST(TimingTests, ElapsedMs)
{
    uint32_t start = utils::get_tick_ms();
    utils::delay_ms(50);
    
    uint32_t elapsed = utils::elapsed_ms(start);
    
    // Should be around 50ms (±20ms tolerance)
    EXPECT_GE(elapsed, 40u);
    EXPECT_LE(elapsed, 70u);
}

// Test elapsed time with wraparound
TEST(TimingTests, ElapsedMsWraparound)
{
    // Simulate wraparound scenario
    uint32_t start = 0xFFFFFFF0;  // Near max uint32_t
    uint32_t end = 0x00000010;    // Wrapped around
    
    uint32_t elapsed = utils::elapsed_ms(start, end);
    
    // Should correctly calculate: 0x00000010 - 0xFFFFFFF0 = 0x20 = 32
    EXPECT_EQ(elapsed, 32u);
}

// Test timeout detection
TEST(TimingTests, HasTimeout)
{
    uint32_t start = utils::get_tick_ms();
    
    // Should not timeout immediately
    EXPECT_FALSE(utils::has_timeout(start, 100));
    
    // Wait and check timeout
    utils::delay_ms(150);
    EXPECT_TRUE(utils::has_timeout(start, 100));
}

// Test timeout with zero timeout
TEST(TimingTests, HasTimeoutZero)
{
    uint32_t start = utils::get_tick_ms();
    
    // Zero timeout should always be true (or immediately true)
    EXPECT_TRUE(utils::has_timeout(start, 0));
}

// Test multiple consecutive delays
TEST(TimingTests, MultipleDelays)
{
    uint32_t start = utils::get_tick_ms();
    
    for (int i = 0; i < 5; i++)
    {
        utils::delay_ms(20);
    }
    
    uint32_t elapsed = utils::elapsed_ms(start);
    
    // Total should be around 100ms (5 * 20ms)
    EXPECT_GE(elapsed, 90u);
    EXPECT_LE(elapsed, 130u);
}

// Test microsecond delay (if available)
TEST(TimingTests, DelayMicroseconds)
{
    uint32_t start = utils::get_tick_ms();
    
    // Delay for 10,000 microseconds (10ms)
    utils::delay_us(10000);
    
    uint32_t elapsed = utils::elapsed_ms(start);
    
    // Should be around 10ms (larger tolerance for microsecond delays)
    EXPECT_GE(elapsed, 5u);
    EXPECT_LE(elapsed, 30u);
}

// Test timing accuracy over longer period
TEST(TimingTests, LongerDelay)
{
    uint32_t start = utils::get_tick_ms();
    
    utils::delay_ms(500);  // 500ms delay
    
    uint32_t elapsed = utils::elapsed_ms(start);
    
    // Should be around 500ms (±50ms tolerance)
    EXPECT_GE(elapsed, 450u);
    EXPECT_LE(elapsed, 550u);
}

// Test that elapsed_ms with no second parameter uses current time
TEST(TimingTests, ElapsedMsDefaultParameter)
{
    uint32_t start = utils::get_tick_ms();
    utils::delay_ms(50);
    
    // Call without second parameter
    uint32_t elapsed = utils::elapsed_ms(start);
    
    EXPECT_GE(elapsed, 40u);
    EXPECT_LE(elapsed, 70u);
}

// Performance test: ensure get_tick_ms is fast
TEST(TimingTests, GetTickMsPerformance)
{
    const int iterations = 10000;
    
    uint32_t start = utils::get_tick_ms();
    
    for (int i = 0; i < iterations; i++)
    {
        volatile uint32_t tick = utils::get_tick_ms();
        (void)tick;  // Prevent optimization
    }
    
    uint32_t elapsed = utils::elapsed_ms(start);
    
    // Should complete very quickly (< 100ms for 10k calls)
    EXPECT_LT(elapsed, 100u);
}
