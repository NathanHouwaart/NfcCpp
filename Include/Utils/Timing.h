/**
 * @file Timing.h
 * @brief Platform-agnostic timing and delay utilities
 * @details Provides cross-platform timing functions that work on both
 *          embedded systems and desktop platforms like Windows.
 * 
 * Usage:
 *   #include "Utils/Timing.h"
 *   
 *   utils::delay_ms(100);           // Delay for 100 milliseconds
 *   utils::delay_us(500);           // Delay for 500 microseconds
 *   
 *   auto start = utils::get_tick_ms();
 *   // ... do work ...
 *   auto elapsed = utils::get_tick_ms() - start;
 */

#ifndef UTILS_TIMING_H
#define UTILS_TIMING_H

#include <stdint.h>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__linux__) || defined(__unix__)
    #define PLATFORM_LINUX
#elif defined(ARDUINO)
    #define PLATFORM_ARDUINO
#elif defined(__arm__) || defined(__thumb__)
    #define PLATFORM_EMBEDDED_ARM
#else
    #define PLATFORM_GENERIC
#endif

// Include platform-specific headers
#if defined(PLATFORM_WINDOWS)
    #include <windows.h>
    #include <thread>
    #include <chrono>
#elif defined(PLATFORM_LINUX)
    #include <unistd.h>
    #include <time.h>
#elif defined(PLATFORM_ARDUINO)
    #include <Arduino.h>
#elif defined(PLATFORM_EMBEDDED_ARM)
    // For bare-metal ARM, you'll need to provide your own implementation
    // based on your hardware timer (e.g., SysTick, HAL_Delay for STM32, etc.)
    extern "C" {
        // Forward declare platform-specific functions that you must implement
        void platform_delay_ms(uint32_t milliseconds);
        void platform_delay_us(uint32_t microseconds);
        uint32_t platform_get_tick_ms(void);
    }
#endif

namespace utils {

/**
 * @brief Delay execution for the specified number of milliseconds
 * @param milliseconds Number of milliseconds to delay
 */
inline void delay_ms(uint32_t milliseconds)
{
#if defined(PLATFORM_WINDOWS)
    // Use std::this_thread::sleep_for on Windows
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    
#elif defined(PLATFORM_LINUX)
    // Use usleep on Linux (takes microseconds)
    usleep(milliseconds * 1000);
    
#elif defined(PLATFORM_ARDUINO)
    // Use Arduino's delay function
    delay(milliseconds);
    
#elif defined(PLATFORM_EMBEDDED_ARM)
    // Call platform-specific implementation
    platform_delay_ms(milliseconds);
    
#else
    // Fallback: busy-wait (not recommended for production)
    // You should implement platform_delay_ms for your target
    #warning "No platform-specific delay implementation, using busy-wait"
    volatile uint32_t count = milliseconds * 1000;
    while (count--) {
        __asm__ __volatile__("nop");
    }
#endif
}

/**
 * @brief Delay execution for the specified number of microseconds
 * @param microseconds Number of microseconds to delay
 * @note On some platforms, this may have limited precision
 */
inline void delay_us(uint32_t microseconds)
{
#if defined(PLATFORM_WINDOWS)
    // Windows has limited precision for microsecond delays
    // For very short delays, this may be less accurate
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    
#elif defined(PLATFORM_LINUX)
    // Use usleep on Linux
    usleep(microseconds);
    
#elif defined(PLATFORM_ARDUINO)
    // Use Arduino's delayMicroseconds function
    delayMicroseconds(microseconds);
    
#elif defined(PLATFORM_EMBEDDED_ARM)
    // Call platform-specific implementation
    platform_delay_us(microseconds);
    
#else
    // Fallback: busy-wait
    #warning "No platform-specific microsecond delay implementation"
    volatile uint32_t count = microseconds;
    while (count--) {
        __asm__ __volatile__("nop");
    }
#endif
}

/**
 * @brief Get current system tick count in milliseconds
 * @return Current tick count in milliseconds
 * @note The tick count may wrap around after ~49 days on 32-bit systems
 */
inline uint32_t get_tick_ms()
{
#if defined(PLATFORM_WINDOWS)
    // Use high-resolution clock
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
    );
    
#elif defined(PLATFORM_LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint32_t>((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
    
#elif defined(PLATFORM_ARDUINO)
    return millis();
    
#elif defined(PLATFORM_EMBEDDED_ARM)
    return platform_get_tick_ms();
    
#else
    #warning "No platform-specific tick implementation"
    return 0;
#endif
}

/**
 * @brief Calculate elapsed time since a start time, handling wraparound
 * @param start_tick Start tick count (from get_tick_ms)
 * @param current_tick Current tick count (from get_tick_ms), or 0 to use current time
 * @return Elapsed time in milliseconds
 */
inline uint32_t elapsed_ms(uint32_t start_tick, uint32_t current_tick = 0)
{
    if (current_tick == 0) {
        current_tick = get_tick_ms();
    }
    
    // Handle wraparound correctly
    return current_tick - start_tick;
}

/**
 * @brief Check if a timeout has occurred
 * @param start_tick Start tick count (from get_tick_ms)
 * @param timeout_ms Timeout duration in milliseconds
 * @return true if timeout has occurred, false otherwise
 */
inline bool has_timeout(uint32_t start_tick, uint32_t timeout_ms)
{
    return elapsed_ms(start_tick) >= timeout_ms;
}

} // namespace utils

#endif // UTILS_TIMING_H
