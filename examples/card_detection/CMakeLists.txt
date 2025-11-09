# Card Detection Example

add_executable(card_detection_example main.cpp)

target_link_libraries(card_detection_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

# Set output directory
set_target_properties(card_detection_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
