# Quick Start Guide

This guide will help you get started with building and using the NfcCpp library.

## Step 1: Clone and Initialize

```powershell
# Clone the repository
git clone <your-repo-url> NfcCpp
cd NfcCpp

# Initialize submodules (required!)
git submodule update --init --recursive
```

## Step 2: Build the Library

```powershell
# Configure the project
cmake -B build -S .

# Build everything (library + examples)
cmake --build build

# Or build specific target
cmake --build build --target NfcCpp
```

## Step 3: Run Examples

```powershell
# Run an example
.\build\examples\Debug\basic_pn532_example.exe
```

## Step 4: Use in Your Project

### Create a new project

```
MyProject/
├── CMakeLists.txt
├── main.cpp
└── NfcCpp/  (clone or add as submodule)
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyNfcApp)

set(CMAKE_CXX_STANDARD 20)

# Option 1: Use as subdirectory
set(NFCCPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(NfcCpp)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE NfcCpp::NfcCpp)
```

### main.cpp

```cpp
#include <iostream>
#include "Pn532/Pn532Driver.h"

int main() {
    std::cout << "My NFC Application\n";
    // Your code here
    return 0;
}
```

### Build your project

```powershell
cd MyProject
cmake -B build -S .
cmake --build build
.\build\Debug\my_app.exe
```

## Common Build Issues

### Submodules not initialized
```
Error: Cannot find etl, tiny-aes, or cppDes
Solution: Run 'git submodule update --init --recursive'
```

### Wrong C++ standard
```
Error: C++20 features not available
Solution: Ensure your compiler supports C++20
```

### CMake version too old
```
Error: CMake 3.16 or higher required
Solution: Update CMake
```

## Build Configurations

### Debug build
```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### Release build
```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Static library (default)
```powershell
cmake -B build -S . -DNFCCPP_BUILD_SHARED_LIBS=OFF
```

### Shared library
```powershell
cmake -B build -S . -DNFCCPP_BUILD_SHARED_LIBS=ON
```

## Installing the Library

```powershell
# Install to default location
cmake --install build

# Or specify install location
cmake --install build --prefix C:/MyLibraries/NfcCpp
```

After installation, use in other projects:

```cmake
find_package(NfcCpp REQUIRED)
target_link_libraries(my_app PRIVATE NfcCpp::NfcCpp)
```

## Next Steps

- Check out the [examples](examples/README.md) for usage patterns
- Read the [main README](README.md) for detailed documentation
- Explore the headers in `Include/` to see available APIs
