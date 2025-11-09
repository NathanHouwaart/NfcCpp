# CMake Project Structure - Implementation Summary

This document describes the complete CMake structure implemented for the NfcCpp library.

## Overview

The project has been reorganized into a proper library structure with:
- ✅ Modular CMake files for each subdirectory
- ✅ Installable library target
- ✅ Export configuration for use in other projects
- ✅ Examples with separate executables
- ✅ Proper dependency management

## File Structure

```
NfcCpp/
│
├── CMakeLists.txt                          # Main project configuration
├── README.md                               # Project documentation
├── QUICKSTART.md                           # Quick start guide
│
├── cmake/
│   └── NfcCppConfig.cmake.in              # Package config template
│
├── Include/                                # Public headers (no CMakeLists needed)
│   ├── Comms/
│   │   ├── BusPoperties.hpp
│   │   ├── IHardwareBus.hpp
│   │   └── Serial/
│   │       ├── ISerialBus.hpp
│   │       └── SerialBusWin.hpp
│   ├── Error/
│   │   └── *.h
│   ├── Nfc/
│   │   ├── Apdu/
│   │   │   └── *.h
│   │   └── Card/
│   │       └── *.h
│   └── Pn532/
│       └── Pn532Driver.h
│
├── Src/
│   ├── CMakeLists.txt                     # Library target definition
│   │
│   ├── Comms/
│   │   ├── CMakeLists.txt                 # Comms module
│   │   └── Serial/
│   │       ├── CMakeLists.txt             # Serial submodule
│   │       └── SerialBusWin.cpp
│   │
│   ├── Pn532/
│   │   ├── CMakeLists.txt                 # PN532 module
│   │   └── Pn532Driver.cpp
│   │
│   └── Utils/
│       ├── CMakeLists.txt                 # Utils module
│       └── cmac.cpp
│
├── examples/
│   ├── CMakeLists.txt                     # Examples configuration
│   ├── README.md                          # Examples documentation
│   │
│   ├── basic_pn532/
│   │   ├── CMakeLists.txt                 # Example executable
│   │   └── main.cpp
│   │
│   ├── serial_communication/
│   │   ├── CMakeLists.txt                 # Example executable
│   │   └── main.cpp
│   │
│   └── card_detection/
│       ├── CMakeLists.txt                 # Example executable
│       └── main.cpp
│
└── external/                               # Third-party dependencies
    ├── etl/
    ├── tiny-aes/
    └── cppDes/
```

## CMake Files Hierarchy

### 1. Root CMakeLists.txt
**Purpose:** Main project configuration, external dependencies, installation rules

**Key Features:**
- Project definition and C++20 standard
- Build options (NFCCPP_BUILD_EXAMPLES, NFCCPP_BUILD_SHARED_LIBS)
- External dependencies (ETL, Tiny-AES, CppDes)
- Subdirectory inclusion (Src, examples)
- Installation and export configuration

**Usage:**
```bash
cmake -B build -S .
cmake --build build
```

### 2. Src/CMakeLists.txt
**Purpose:** Define the NfcCpp library target

**Key Features:**
- Includes all module subdirectories
- Creates main library target (STATIC or SHARED)
- Defines public/private include directories
- Links external dependencies
- Creates NfcCpp::NfcCpp alias

**Target:** `NfcCpp` library

### 3. Module CMakeLists (Src/*/CMakeLists.txt)
**Purpose:** Define object libraries for each module

**Modules:**
- **Src/Comms/CMakeLists.txt** - Communication interfaces
- **Src/Comms/Serial/CMakeLists.txt** - Serial communication
- **Src/Pn532/CMakeLists.txt** - PN532 driver
- **Src/Utils/CMakeLists.txt** - Utility functions

**Pattern:**
```cmake
add_library(NfcCpp_ModuleName OBJECT)
target_sources(NfcCpp_ModuleName PRIVATE file.cpp)
target_include_directories(...)
target_link_libraries(...)
```

**Targets:** `NfcCpp_Comms`, `NfcCpp_Pn532`, `NfcCpp_Utils`, `NfcCpp_Comms_Serial`

### 4. Examples CMakeLists (examples/*/CMakeLists.txt)
**Purpose:** Define example executables

**Structure:**
- `examples/CMakeLists.txt` - Includes all example subdirectories
- Each example has its own CMakeLists.txt

**Pattern:**
```cmake
add_executable(example_name main.cpp)
target_link_libraries(example_name PRIVATE NfcCpp::NfcCpp)
```

**Targets:** `basic_pn532_example`, `serial_communication_example`, `card_detection_example`

## Build Targets

| Target | Type | Description |
|--------|------|-------------|
| `NfcCpp` | Library | Main library (static/shared) |
| `NfcCpp::NfcCpp` | Alias | Namespaced alias for consistency |
| `NfcCpp_Comms` | Object Library | Communications module |
| `NfcCpp_Comms_Serial` | Object Library | Serial communication |
| `NfcCpp_Pn532` | Object Library | PN532 driver module |
| `NfcCpp_Utils` | Object Library | Utilities module |
| `basic_pn532_example` | Executable | PN532 example |
| `serial_communication_example` | Executable | Serial example |
| `card_detection_example` | Executable | Card detection example |

## Build Options

```cmake
# Enable/disable examples (default: ON)
-DNFCCPP_BUILD_EXAMPLES=ON/OFF

# Build as shared library (default: OFF = static)
-DNFCCPP_BUILD_SHARED_LIBS=ON/OFF
```

## Usage in Other Projects

### Method 1: Add as Subdirectory

```cmake
# In your CMakeLists.txt
add_subdirectory(path/to/NfcCpp)
target_link_libraries(your_target PRIVATE NfcCpp::NfcCpp)
```

### Method 2: Install and Use find_package

```bash
# Install NfcCpp
cmake --install build --prefix /install/path
```

```cmake
# In your CMakeLists.txt
find_package(NfcCpp REQUIRED)
target_link_libraries(your_target PRIVATE NfcCpp::NfcCpp)
```

## Adding New Components

### New Source Module

1. Create directory under `Src/NewModule/`
2. Add source files
3. Create `Src/NewModule/CMakeLists.txt`:
```cmake
add_library(NfcCpp_NewModule OBJECT)
target_sources(NfcCpp_NewModule PRIVATE file.cpp)
target_include_directories(NfcCpp_NewModule PUBLIC ${CMAKE_SOURCE_DIR}/Include)
```
4. Add to `Src/CMakeLists.txt`:
```cmake
add_subdirectory(NewModule)
# ... in target_sources:
$<TARGET_OBJECTS:NfcCpp_NewModule>
```

### New Example

1. Create `examples/new_example/` directory
2. Add `main.cpp`
3. Create `examples/new_example/CMakeLists.txt`:
```cmake
add_executable(new_example main.cpp)
target_link_libraries(new_example PRIVATE NfcCpp::NfcCpp)
set_target_properties(new_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
```
4. Add to `examples/CMakeLists.txt`:
```cmake
add_subdirectory(new_example)
```

## Dependencies

The library automatically handles these dependencies:
- **ETL** - Embedded Template Library (header-only)
- **Tiny-AES** - AES encryption library
- **CppDes** - DES/3DES encryption library

All dependencies are included as submodules and built automatically.

## Installation

When installed, the following are provided:
- Library files (`libnfccpp.a` or `nfccpp.dll`)
- Header files (in `include/NfcCpp/`)
- CMake config files (in `lib/cmake/NfcCpp/`)
- Export targets (NfcCppTargets.cmake)

This allows other projects to use:
```cmake
find_package(NfcCpp REQUIRED)
```

## Summary

✅ **Modular Structure** - Each directory has its own CMakeLists.txt
✅ **Library Target** - NfcCpp can be used in other projects
✅ **Examples** - Separate executables demonstrating usage
✅ **Installable** - Can be installed system-wide or locally
✅ **Flexible** - Options for static/shared libs and examples
✅ **Modern CMake** - Uses targets, namespaces, and proper linking
