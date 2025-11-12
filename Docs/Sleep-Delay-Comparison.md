# Sleep/Delay Implementation Comparison

## Quick Answer

**ETL does NOT provide sleep/delay functions** - it only provides time representation (like `std::chrono` does).

For cross-platform sleep/delay, use the provided **`Utils/Timing.h`** utilities.

---

## Comparison Table

| Feature | `std::chrono` | `etl::chrono` | `Utils/Timing.h` |
|---------|---------------|---------------|------------------|
| **Time representation** | ✅ Yes | ✅ Yes | ❌ No (simple) |
| **Sleep/delay** | ❌ No* | ❌ No | ✅ Yes |
| **Embedded support** | ❌ Limited | ✅ Full | ✅ Full |
| **Platform-agnostic** | ⚠️ Partial | ✅ Yes | ✅ Yes |
| **Thread support needed** | ⚠️ For sleep | ❌ No | ❌ No |
| **Tick counting** | ✅ Yes | ✅ Yes | ✅ Yes |

\* *Requires `std::this_thread::sleep_for` from `<thread>` header*

---

## What Each Provides

### `std::chrono` (C++ Standard)

**Provides:**
- Duration types (`milliseconds`, `seconds`, etc.)
- Time points and clocks
- Time arithmetic

**Does NOT provide:**
- Actual sleep/delay (needs `std::this_thread::sleep_for`)

**Limitations:**
- Not available on many embedded systems
- Requires thread support for sleep

**Example:**
```cpp
#include <chrono>
#include <thread>

// Time representation
auto duration = std::chrono::milliseconds(100);

// Actual sleep (requires <thread>)
std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

---

### `etl::chrono` (Embedded Template Library)

**Provides:**
- Duration types (ETL versions)
- Time points and clocks
- Time arithmetic
- **Platform-agnostic time representation**

**Does NOT provide:**
- Sleep/delay functions
- Platform-specific timing APIs

**Why?**
ETL intentionally avoids platform-specific code. Sleep/delay mechanisms vary:
- Windows: `Sleep()` or `std::this_thread::sleep_for`
- Linux: `usleep()` or `nanosleep()`
- Arduino: `delay()`
- STM32: `HAL_Delay()`
- FreeRTOS: `vTaskDelay()`
- Bare-metal: Hardware timers

**Example:**
```cpp
#include "etl/chrono.h"

// Time representation
etl::chrono::milliseconds duration(100);

// But no way to actually sleep for that duration!
// etl::this_thread::sleep_for(duration);  // ❌ Doesn't exist
```

---

### `Utils/Timing.h` (This Project)

**Provides:**
- ✅ `delay_ms()` - Cross-platform millisecond delay
- ✅ `delay_us()` - Cross-platform microsecond delay
- ✅ `get_tick_ms()` - Platform-agnostic tick counter
- ✅ `elapsed_ms()` - Time difference calculation
- ✅ `has_timeout()` - Timeout checking

**Platform support:**
- Windows
- Linux/Unix
- Arduino
- Embedded ARM (with custom implementation)

**Example:**
```cpp
#include "Utils/Timing.h"

// Simple delay
utils::delay_ms(100);

// Timeout pattern
uint32_t start = utils::get_tick_ms();
while (!utils::has_timeout(start, 5000))
{
    // Wait with timeout
}
```

---

## When to Use What

### Use `std::chrono` when:
- ✅ Desktop/server applications only
- ✅ Need precise time arithmetic
- ✅ Thread support is available
- ✅ Using standard C++ features

### Use `etl::chrono` when:
- ✅ Need time representation on embedded
- ✅ Calculating durations/time differences
- ✅ Using other ETL containers
- ❌ **But pair it with `Utils/Timing.h` for actual delays!**

### Use `Utils/Timing.h` when:
- ✅ Need actual sleep/delay functionality
- ✅ Cross-platform code (embedded + desktop)
- ✅ Simple timing without overhead
- ✅ Timeout/polling patterns

---

## Migration Guide

### From `std::this_thread::sleep_for`

**Before:**
```cpp
#include <thread>
#include <chrono>

std::this_thread::sleep_for(std::chrono::milliseconds(100));
std::this_thread::sleep_for(std::chrono::seconds(2));
```

**After:**
```cpp
#include "Utils/Timing.h"

utils::delay_ms(100);
utils::delay_ms(2000);
```

### From Platform-Specific APIs

**Before:**
```cpp
// Windows
Sleep(100);

// Linux
usleep(100000);

// Arduino
delay(100);

// STM32
HAL_Delay(100);
```

**After (all platforms):**
```cpp
#include "Utils/Timing.h"

utils::delay_ms(100);
```

---

## Best Practices

### ✅ DO:
```cpp
#include "Utils/Timing.h"

// Simple delays
utils::delay_ms(100);

// Timeouts
uint32_t start = utils::get_tick_ms();
while (!done && !utils::has_timeout(start, 5000)) {
    // Work with timeout protection
}

// Periodic tasks (non-blocking)
static uint32_t last_run = 0;
if (utils::elapsed_ms(last_run) >= 1000) {
    last_run = utils::get_tick_ms();
    periodic_task();
}
```

### ❌ DON'T:
```cpp
// Don't try to sleep with ETL chrono
// etl::this_thread::sleep_for(...);  // Doesn't exist!

// Don't use platform-specific code directly
#ifdef _WIN32
    Sleep(100);
#elif defined(__linux__)
    usleep(100000);
#endif
// Use utils::delay_ms(100) instead!

// Don't calculate time differences manually
uint32_t end = utils::get_tick_ms();
uint32_t duration = end - start;  // ⚠️ Breaks on wraparound!
// Use utils::elapsed_ms(start) instead!
```

---

## Summary

| Need | Solution |
|------|----------|
| Sleep/delay on Windows | `utils::delay_ms()` |
| Sleep/delay on embedded | `utils::delay_ms()` + platform impl |
| Time representation | `etl::chrono` or `std::chrono` |
| Timeout checking | `utils::has_timeout()` |
| Measure elapsed time | `utils::elapsed_ms()` |
| Get current time | `utils::get_tick_ms()` |

**Bottom line:** ETL provides time types, but you need `Utils/Timing.h` for actual delays.
