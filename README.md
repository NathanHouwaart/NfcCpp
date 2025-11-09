# NfcCpp Library

A C++20 library for NFC communication with support for PN532 readers and various NFC card types.

## Features

- PN532 NFC reader driver
- Serial communication interface (Windows)
- Card detection and identification
- APDU command support
- Error handling framework
- Cryptographic utilities (CMAC, DES, AES)

## Project Structure

```
NfcCpp/
├── Include/           # Public header files
│   ├── Comms/        # Communication interfaces
│   ├── Error/        # Error handling
│   ├── Nfc/          # NFC card and APDU interfaces
│   └── Pn532/        # PN532 driver headers
├── Src/              # Implementation files
│   ├── Comms/        # Communication implementation
│   ├── Pn532/        # PN532 driver implementation
│   └── Utils/        # Utility functions
├── examples/         # Example applications
├── external/         # Third-party dependencies
│   ├── etl/          # Embedded Template Library
│   ├── tiny-aes/     # AES encryption
│   └── cppDes/       # DES encryption
└── cmake/            # CMake configuration files
```

## Building the Library

### Prerequisites

- CMake 3.16 or higher
- C++20 compatible compiler (MSVC, GCC, Clang)
- Git (for submodules)

### Clone and Build

```bash
# Clone the repository
git clone <your-repo-url> NfcCpp
cd NfcCpp

# Initialize submodules
git submodule update --init --recursive

# Configure
cmake -B build -S .

# Build
cmake --build build

# Install (optional)
cmake --install build --prefix /path/to/install
```

### Build Options

- `NFCCPP_BUILD_EXAMPLES` - Build example applications (default: ON)
- `NFCCPP_BUILD_SHARED_LIBS` - Build as shared library (default: OFF)

```bash
# Build without examples
cmake -B build -S . -DNFCCPP_BUILD_EXAMPLES=OFF

# Build as shared library
cmake -B build -S . -DNFCCPP_BUILD_SHARED_LIBS=ON
```

## Using the Library in Your Project

### Method 1: Install and Find Package

After installing the library:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject)

# Find the installed library
find_package(NfcCpp REQUIRED)

# Create your executable
add_executable(my_app main.cpp)

# Link against NfcCpp
target_link_libraries(my_app PRIVATE NfcCpp::NfcCpp)
```

### Method 2: Add as Subdirectory

Add NfcCpp as a subdirectory in your project:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject)

# Add NfcCpp (disable examples if you don't need them)
set(NFCCPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/NfcCpp)

# Create your executable
add_executable(my_app main.cpp)

# Link against NfcCpp
target_link_libraries(my_app PRIVATE NfcCpp::NfcCpp)
```

### Example Usage

```cpp
#include <iostream>
#include "Pn532/Pn532Driver.h"
#include "Comms/Serial/SerialBusWin.hpp"

int main() {
    // Your NFC application code here
    return 0;
}
```

## Examples

See the [examples](examples/README.md) directory for complete example applications.

## Dependencies

The library includes the following third-party dependencies as submodules:

- **ETL (Embedded Template Library)** - Header-only template library
- **Tiny-AES** - Small AES implementation
- **CppDes** - DES/3DES implementation

## CMake Structure

Each subdirectory has its own `CMakeLists.txt` for modular organization:

- **Root CMakeLists.txt** - Main project configuration, dependencies, and installation
- **Src/CMakeLists.txt** - Library target definition
- **Src/[Module]/CMakeLists.txt** - Individual module object libraries
- **examples/CMakeLists.txt** - Examples configuration
- **examples/[Example]/CMakeLists.txt** - Individual example executables

## Platform Support

Currently supported platforms:
- Windows (with MSVC, MinGW, or Clang)

## License

[Your license here]

## Contributing

[Your contribution guidelines here]

## Contact

[Your contact information here]
