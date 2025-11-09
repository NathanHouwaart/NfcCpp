# Basic PN532 Example

add_executable(basic_pn532_example main.cpp)

target_link_libraries(basic_pn532_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

# Set output directory
set_target_properties(basic_pn532_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
