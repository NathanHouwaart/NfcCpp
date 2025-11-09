# NfcCpp Tests

This directory contains unit and integration tests for the NfcCpp library using Google Test.

## Test Structure

```
tests/
├── unit/                  # Unit tests (no hardware required)
│   ├── SerialBusTests.cpp    # Serial interface tests
│   ├── ErrorTests.cpp        # Error handling tests
│   └── Pn532Tests.cpp        # PN532 driver tests
└── integration/           # Integration tests (require hardware)
    └── SerialIntegrationTest.cpp
```

## Building Tests

Tests are built by default. To disable:

```powershell
cmake -B build -S . -DNFCCPP_BUILD_TESTS=OFF
cmake --build build
```

## Running Tests

### Run All Tests

```powershell
# Build first
cmake --build build

# Run all tests
cd build
ctest --output-on-failure
```

### Run Specific Test

```powershell
# Run only serial bus tests
.\build\tests\unit\test_serial_bus.exe

# Run only error tests
.\build\tests\unit\test_error.exe

# Run only PN532 tests
.\build\tests\unit\test_pn532.exe
```

### Run with Verbose Output

```powershell
# Show detailed test output
.\build\tests\unit\test_serial_bus.exe --gtest_print_time=1
```

### Run Tests with Filters

```powershell
# Run only tests matching a pattern
.\build\tests\unit\test_serial_bus.exe --gtest_filter=SerialBusTest.Construction

# Run all tests except specific ones
.\build\tests\unit\test_serial_bus.exe --gtest_filter=-*Invalid*
```

## Unit Tests

Unit tests don't require actual hardware. They test:

- **SerialBusTests**: Serial bus interface logic, error handling, and API
- **ErrorTests**: Error type creation, conversion, and string representation
- **Pn532Tests**: PN532 driver with mocked hardware bus

### Example Unit Test Output

```
[==========] Running 10 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 10 tests from SerialBusTest
[ RUN      ] SerialBusTest.Construction
[       OK ] SerialBusTest.Construction (0 ms)
[ RUN      ] SerialBusTest.PortName
[       OK ] SerialBusTest.PortName (0 ms)
...
[==========] 10 tests from 1 test suite ran. (15 ms total)
[  PASSED  ] 10 tests.
```

## Integration Tests

Integration tests require actual hardware and are **DISABLED by default**.

### Enable Integration Tests

```powershell
cmake -B build -S . -DBUILD_INTEGRATION_TESTS=ON
cmake --build build
```

### Run Integration Tests

```powershell
# Run with disabled tests enabled
.\build\tests\integration\test_serial_integration.exe --gtest_also_run_disabled_tests
```

**Important**: Update the COM port in `SerialIntegrationTest.cpp` before running:

```cpp
const char* testPort = "COM3";  // Change to your port
```

## Writing New Tests

### Add a New Unit Test

1. Create `tests/unit/MyNewTest.cpp`:

```cpp
#include <gtest/gtest.h>
#include "MyModule/MyClass.h"

TEST(MyNewTest, BasicTest) {
    MyClass obj;
    EXPECT_TRUE(obj.isValid());
}
```

2. Add to `tests/unit/CMakeLists.txt`:

```cmake
add_executable(test_mynew
    MyNewTest.cpp
)

target_link_libraries(test_mynew
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

add_test(NAME MyNewTests COMMAND test_mynew)
```

### Test Macros

```cpp
// Basic assertions
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);
EXPECT_EQ(val1, val2);
EXPECT_NE(val1, val2);
EXPECT_LT(val1, val2);
EXPECT_GT(val1, val2);

// Fatal assertions (stop test on failure)
ASSERT_TRUE(condition);
ASSERT_EQ(val1, val2);

// String comparisons
EXPECT_STREQ(str1, str2);

// Exception testing
EXPECT_THROW(statement, exception_type);
EXPECT_NO_THROW(statement);
```

### Test Fixtures

For shared setup/teardown:

```cpp
class MyTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Runs before each test
        myObject = new MyClass();
    }
    
    void TearDown() override {
        // Runs after each test
        delete myObject;
    }
    
    MyClass* myObject;
};

TEST_F(MyTestFixture, TestWithFixture) {
    EXPECT_NE(myObject, nullptr);
}
```

## Mock Objects

The `Pn532Tests.cpp` demonstrates creating mock objects:

```cpp
class MockHardwareBus : public comms::IHardwareBus {
public:
    // Implement interface with test-friendly behavior
    etl::expected<void, Error> write(const etl::ivector<uint8_t>& data) override {
        lastWrittenData = data;
        return {};
    }
    
    // Add helper methods for testing
    etl::vector<uint8_t, 256> getLastWrittenData() const {
        return lastWrittenData;
    }
private:
    etl::vector<uint8_t, 256> lastWrittenData;
};
```

## Continuous Integration

Tests can be integrated into CI/CD:

```yaml
# GitHub Actions example
- name: Build and Test
  run: |
    cmake -B build -S .
    cmake --build build
    cd build
    ctest --output-on-failure
```

## Test Coverage

To generate code coverage (requires additional tools):

```powershell
# Using gcov/lcov (MinGW)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build
ctest
# Generate coverage report with lcov
```

## Tips

1. **Keep tests fast**: Unit tests should run in milliseconds
2. **Test one thing**: Each test should verify one specific behavior
3. **Use descriptive names**: `TEST(Module, DescriptiveTestName)`
4. **Mock external dependencies**: Don't rely on real hardware for unit tests
5. **Test edge cases**: Empty inputs, null pointers, boundary conditions
6. **Use fixtures**: Share common setup code across tests

## Troubleshooting

### Tests Won't Build

```powershell
# Clean and rebuild
Remove-Item -Recurse -Force build
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build
```

### Google Test Not Found

Google Test is fetched automatically by the cppDes dependency. If issues occur, it will be available after building cppDes.

### Tests Fail on CI

Disable hardware-dependent tests in CI:

```cmake
# In test CMakeLists.txt
if(NOT CI_BUILD)
    add_test(NAME HardwareTest COMMAND test_hardware)
endif()
```
