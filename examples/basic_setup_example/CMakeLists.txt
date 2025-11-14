# basic_setup_example - Demonstrates CardManager usage for card detection and session management

add_executable(basic_setup_example
    main.cpp
)

target_include_directories(basic_setup_example 
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include

)

target_link_libraries(basic_setup_example PRIVATE
    NfcCpp
    etl::etl
)

# Set output directory
set_target_properties(basic_setup_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)


# On Windows, copy any required DLLs
if(WIN32)
    # Add any Windows-specific settings here
endif()
