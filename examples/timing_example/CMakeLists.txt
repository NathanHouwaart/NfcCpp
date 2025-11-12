# Timing Example
add_executable(timing_example
    main.cpp
)

target_link_libraries(timing_example PRIVATE
    NfcCpp
)

# Set output directory
set_target_properties(timing_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation
install(TARGETS timing_example
    RUNTIME DESTINATION bin
)
