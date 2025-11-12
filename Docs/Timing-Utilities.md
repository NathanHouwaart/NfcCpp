# Cross-Platform Timing Utilities

## Overview

The `Utils/Timing.h` header provides platform-agnostic timing and delay functions that work across Windows, Linux, Arduino, and embedded ARM platforms.

## Why Not Use ETL's chrono?

**ETL's `etl::chrono`** is designed for time representation (durations, time points) but **does not provide platform-specific sleep/delay functions** because:
- ETL is platform-agnostic and avoids OS-specific calls
- Sleep/delay mechanisms vary widely between platforms (Windows APIs, POSIX, bare-metal timers)
- Embedded systems may not have thread support for `std::this_thread::sleep_for`

## Supported Platforms

| Platform | Detection | Implementation |
|----------|-----------|----------------|
| **Windows** | `_WIN32` or `_WIN64` | `std::this_thread::sleep_for` + `std::chrono` |
| **Linux/Unix** | `__linux__` or `__unix__` | `usleep()` + `clock_gettime()` |
| **Arduino** | `ARDUINO` | `delay()`, `delayMicroseconds()`, `millis()` |
| **Embedded ARM** | `__arm__` or `__thumb__` | Custom implementation required |

## API Reference

### Functions

#### `void delay_ms(uint32_t milliseconds)`
Blocks execution for the specified number of milliseconds.

```cpp
#include "Utils/Timing.h"

utils::delay_ms(100);  // Wait 100ms
```

#### `void delay_us(uint32_t microseconds)`
Blocks execution for the specified number of microseconds.

```cpp
utils::delay_us(500);  // Wait 500μs
```

> **Note:** Microsecond precision may be limited on some platforms (e.g., Windows).

#### `uint32_t get_tick_ms()`
Returns the current system tick count in milliseconds.

```cpp
uint32_t start = utils::get_tick_ms();
// ... do work ...
uint32_t end = utils::get_tick_ms();
uint32_t duration = end - start;
```

> **Note:** The tick count wraps around after ~49 days (for 32-bit systems).

#### `uint32_t elapsed_ms(uint32_t start_tick, uint32_t current_tick = 0)`
Calculates elapsed time since `start_tick`, handling wraparound correctly.

```cpp
uint32_t start = utils::get_tick_ms();
// ... do work ...
uint32_t elapsed = utils::elapsed_ms(start);
```

#### `bool has_timeout(uint32_t start_tick, uint32_t timeout_ms)`
Checks if a timeout has occurred.

```cpp
uint32_t start = utils::get_tick_ms();
while (!utils::has_timeout(start, 5000))  // 5 second timeout
{
    // Do work with timeout protection
    if (/* work complete */) break;
}
```

## Usage Examples

### Basic Delay

```cpp
#include "Utils/Timing.h"

void setup() {
    // Initialize hardware
}

void loop() {
    // Toggle LED every second
    toggle_led();
    utils::delay_ms(1000);
}
```

### Timeout Pattern

```cpp
#include "Utils/Timing.h"

bool wait_for_response(uint32_t timeout_ms)
{
    uint32_t start = utils::get_tick_ms();
    
    while (!utils::has_timeout(start, timeout_ms))
    {
        if (data_available()) {
            return true;  // Success
        }
        utils::delay_ms(10);  // Small delay between checks
    }
    
    return false;  // Timeout
}
```

### Performance Measurement

```cpp
#include "Utils/Timing.h"
#include <iostream>

void measure_operation()
{
    uint32_t start = utils::get_tick_ms();
    
    perform_expensive_operation();
    
    uint32_t duration = utils::elapsed_ms(start);
    std::cout << "Operation took: " << duration << " ms" << std::endl;
}
```

### Non-Blocking Delay

```cpp
#include "Utils/Timing.h"

class PeriodicTask
{
private:
    uint32_t interval_ms;
    uint32_t last_run;
    
public:
    PeriodicTask(uint32_t interval) 
        : interval_ms(interval)
        , last_run(utils::get_tick_ms())
    {}
    
    bool should_run()
    {
        if (utils::elapsed_ms(last_run) >= interval_ms)
        {
            last_run = utils::get_tick_ms();
            return true;
        }
        return false;
    }
};

// Usage
PeriodicTask task(1000);  // Run every second

void loop()
{
    if (task.should_run())
    {
        // Execute task
        do_periodic_work();
    }
    
    // Do other work without blocking
}
```

## Embedded Platform Setup

### For STM32 with HAL

1. Edit `Src/Utils/TimingEmbedded.cpp`
2. Uncomment the STM32 HAL section
3. Add to your `CMakeLists.txt`:

```cmake
if(TARGET_PLATFORM STREQUAL "STM32")
    target_sources(YourLib PRIVATE
        Src/Utils/TimingEmbedded.cpp
    )
endif()
```

### For FreeRTOS

```cpp
extern "C" {

void platform_delay_ms(uint32_t milliseconds)
{
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
}

uint32_t platform_get_tick_ms(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

}
```

### For Bare-Metal ARM

Configure SysTick to interrupt every 1ms, then implement:

```cpp
volatile uint32_t g_systick_count = 0;

extern "C" void SysTick_Handler(void)
{
    g_systick_count++;
}

extern "C" void platform_delay_ms(uint32_t milliseconds)
{
    uint32_t start = g_systick_count;
    while ((g_systick_count - start) < milliseconds);
}

extern "C" uint32_t platform_get_tick_ms(void)
{
    return g_systick_count;
}
```

## Comparison with Alternatives

| Approach | Pros | Cons |
|----------|------|------|
| **`utils::delay_ms()`** | ✅ Cross-platform<br>✅ No external deps<br>✅ Works on embedded | ❌ Requires platform setup |
| **`std::this_thread::sleep_for`** | ✅ Standard C++<br>✅ Works on desktop | ❌ No embedded support<br>❌ Requires thread support |
| **`etl::chrono`** | ✅ Time representation<br>✅ ETL integration | ❌ No sleep/delay functions |
| **Platform-specific** | ✅ Native performance | ❌ Not portable |

## Best Practices

1. **Use `delay_ms()` for delays > 1ms** - More efficient than busy-waiting
2. **Use timeouts with `has_timeout()`** - Prevent infinite loops
3. **Prefer non-blocking patterns** - Use `elapsed_ms()` for periodic tasks
4. **Handle wraparound** - Always use `elapsed_ms()` for time differences
5. **Choose appropriate precision** - Use `delay_us()` only when needed

## Migration from `std::chrono`

**Before:**
```cpp
#include <thread>
#include <chrono>

std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

**After:**
```cpp
#include "Utils/Timing.h"

utils::delay_ms(100);
```

## License

Same as NfcCpp project.
