# NfcCpp Examples

This directory contains example applications demonstrating how to use the NfcCpp library.

## Building Examples

The examples are built automatically when you build the main project with the `NFCCPP_BUILD_EXAMPLES` option enabled (default):

```bash
cmake -B build -S .
cmake --build build
```

To disable building examples:

```bash
cmake -B build -S . -DNFCCPP_BUILD_EXAMPLES=OFF
cmake --build build
```

## Running Examples

After building, the example executables will be in the `build/examples` directory:

```bash
# On Windows
.\build\examples\basic_pn532_example.exe
.\build\examples\serial_communication_example.exe
.\build\examples\card_detection_example.exe
```

## Available Examples

### 1. Basic PN532 (`basic_pn532`)
Demonstrates basic initialization and usage of the PN532 NFC reader driver.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration

### 2. Serial Communication (`serial_communication`)
Shows how to use the serial communication interface for Windows.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration

### 3. Card Detection (`card_detection`)
Illustrates how to detect and identify NFC cards.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration

## Adding Your Own Example

To create a new example:

1. Create a new directory under `examples/`
2. Create a `main.cpp` file with your example code
3. Create a `CMakeLists.txt` file:

```cmake
add_executable(my_example_name main.cpp)

target_link_libraries(my_example_name
    PRIVATE
        NfcCpp::NfcCpp
)

set_target_properties(my_example_name PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
```

4. Add your example to `examples/CMakeLists.txt`:

```cmake
add_subdirectory(my_example_name)
```

## Notes

- All examples link against the NfcCpp library using the `NfcCpp::NfcCpp` target
- Examples are template code - you'll need to implement the actual logic based on your hardware setup
- Make sure your hardware (PN532 reader, serial ports, etc.) is properly connected before running examples
