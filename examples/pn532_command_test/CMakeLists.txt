# PN532 Command Test Application

# Create executable
add_executable(pn532_command_test main.cpp)

# Link against NfcCpp library and ETL
target_link_libraries(pn532_command_test
    PRIVATE
        NfcCpp
        etl::etl
)

# Include directories
target_include_directories(pn532_command_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

# Set output directory
set_target_properties(pn532_command_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

# On Windows, copy any required DLLs
if(WIN32)
    # Add any Windows-specific settings here
endif()
