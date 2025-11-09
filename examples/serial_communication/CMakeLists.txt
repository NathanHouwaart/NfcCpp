# Serial Communication Example

add_executable(serial_communication_example main.cpp)

target_link_libraries(serial_communication_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

# Set output directory
set_target_properties(serial_communication_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
