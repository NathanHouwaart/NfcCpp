# NfcCpp CMake Build Flow

## Build Dependency Graph

```
Root CMakeLists.txt
│
├─── external/etl/CMakeLists.txt → etl::etl (header-only)
│
├─── external/tiny-aes/CMakeLists.txt → tiny-aes (static lib)
│
├─── external/cppDes/CMakeLists.txt → libcppdes (static lib)
│
├─── Src/CMakeLists.txt → NfcCpp (main library)
│    │
│    ├─── Src/Comms/CMakeLists.txt → NfcCpp_Comms (object lib)
│    │    │
│    │    └─── Src/Comms/Serial/CMakeLists.txt → NfcCpp_Comms_Serial (object lib)
│    │         └── SerialBusWin.cpp
│    │
│    ├─── Src/Pn532/CMakeLists.txt → NfcCpp_Pn532 (object lib)
│    │    └── Pn532Driver.cpp
│    │
│    └─── Src/Utils/CMakeLists.txt → NfcCpp_Utils (object lib)
│         └── cmac.cpp
│
└─── examples/CMakeLists.txt
     │
     ├─── examples/basic_pn532/CMakeLists.txt → basic_pn532_example (executable)
     │    └── main.cpp
     │
     ├─── examples/serial_communication/CMakeLists.txt → serial_communication_example (executable)
     │    └── main.cpp
     │
     └─── examples/card_detection/CMakeLists.txt → card_detection_example (executable)
          └── main.cpp
```

## Link Dependencies

```
NfcCpp (library)
├── PUBLIC: etl::etl
├── PRIVATE: tiny-aes
├── PRIVATE: libcppdes
└── OBJECTS:
    ├── NfcCpp_Comms
    │   └── NfcCpp_Comms_Serial
    ├── NfcCpp_Pn532
    └── NfcCpp_Utils

Each Example (executable)
└── PRIVATE: NfcCpp::NfcCpp
```

## Include Directories Structure

```
Public Headers (accessible to library users):
Include/
├── Comms/
│   ├── BusPoperties.hpp
│   ├── IHardwareBus.hpp
│   └── Serial/
│       ├── ISerialBus.hpp
│       └── SerialBusWin.hpp
├── Error/
│   └── *.h
├── Nfc/
│   ├── Apdu/
│   └── Card/
└── Pn532/
    └── Pn532Driver.h

Private Implementation:
Src/
├── Comms/Serial/
│   └── SerialBusWin.cpp
├── Pn532/
│   └── Pn532Driver.cpp
└── Utils/
    └── cmac.cpp
```

## Build Process Flow

### 1. Configure Phase
```
cmake -B build -S .
│
├── Detect compiler and platform
├── Process root CMakeLists.txt
│   ├── Set C++20 standard
│   ├── Define options
│   └── Process subdirectories:
│       ├── external/etl
│       ├── external/tiny-aes
│       ├── external/cppDes
│       ├── Src/
│       │   ├── Comms/
│       │   │   └── Serial/
│       │   ├── Pn532/
│       │   └── Utils/
│       └── examples/ (if enabled)
│           ├── basic_pn532/
│           ├── serial_communication/
│           └── card_detection/
└── Generate build files
```

### 2. Build Phase
```
cmake --build build
│
├── Build external dependencies:
│   ├── etl (header-only, nothing to build)
│   ├── tiny-aes → tiny-aes.lib/a
│   └── libcppdes → libcppdes.lib/a
│
├── Build object libraries:
│   ├── NfcCpp_Comms_Serial.obj
│   ├── NfcCpp_Comms.obj
│   ├── NfcCpp_Pn532.obj
│   └── NfcCpp_Utils.obj
│
├── Link main library:
│   └── NfcCpp.lib/a (or .dll if shared)
│
└── Build examples (if enabled):
    ├── basic_pn532_example.exe
    ├── serial_communication_example.exe
    └── card_detection_example.exe
```

### 3. Install Phase (optional)
```
cmake --install build --prefix <path>
│
├── Install library: lib/nfccpp.lib|a
├── Install headers: include/NfcCpp/**/*.h|hpp
└── Install CMake config: lib/cmake/NfcCpp/
    ├── NfcCppConfig.cmake
    ├── NfcCppConfigVersion.cmake
    └── NfcCppTargets.cmake
```

## Using the Installed Library

```
Your Project
│
├── CMakeLists.txt
│   find_package(NfcCpp REQUIRED)
│   target_link_libraries(your_app PRIVATE NfcCpp::NfcCpp)
│
└── main.cpp
    #include "Pn532/Pn532Driver.h"
```

When building your project:
```
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/nfccpp/install
cmake --build build
```

## CMake Variables Used

| Variable | Purpose | Default |
|----------|---------|---------|
| `NFCCPP_BUILD_EXAMPLES` | Build example applications | ON |
| `NFCCPP_BUILD_SHARED_LIBS` | Build as shared library | OFF |
| `CMAKE_CXX_STANDARD` | C++ standard version | 20 |
| `CMAKE_BUILD_TYPE` | Build configuration | Debug |
| `CMAKE_INSTALL_PREFIX` | Installation location | Platform default |
