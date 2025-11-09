# Integration Tests (require actual hardware)

# These tests are typically disabled by default
# Enable with -DBUILD_INTEGRATION_TESTS=ON

option(BUILD_INTEGRATION_TESTS "Build integration tests (require hardware)" OFF)

if(BUILD_INTEGRATION_TESTS)
    add_executable(test_serial_integration
        SerialIntegrationTest.cpp
    )
    
    target_link_libraries(test_serial_integration
        PRIVATE
            NfcCpp::NfcCpp
            etl::etl
            gtest
            gtest_main
    )
    
    target_include_directories(test_serial_integration
        PRIVATE
            ${CMAKE_SOURCE_DIR}/Include
    )
    
    # Not registered with CTest by default - run manually
    # add_test(NAME SerialIntegration COMMAND test_serial_integration)
endif()
