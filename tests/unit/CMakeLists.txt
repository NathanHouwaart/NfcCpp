# Unit Tests

# Serial Bus Tests
add_executable(test_serial_bus
    SerialBusTests.cpp
)

target_link_libraries(test_serial_bus
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_serial_bus
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/Src
)

# Register test with CTest
add_test(NAME SerialBusTests COMMAND test_serial_bus)

# Error Handling Tests
add_executable(test_error
    ErrorTests.cpp
)

target_link_libraries(test_error
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_error
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME ErrorTests COMMAND test_error)

# PN532 Driver Tests
add_executable(test_pn532
    Pn532Tests.cpp
)

target_link_libraries(test_pn532
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_pn532
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME Pn532Tests COMMAND test_pn532)
