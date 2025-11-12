# card_manager_example - Demonstrates CardManager usage for card detection and session management

add_executable(card_manager_example
    main.cpp
)

target_include_directories(card_manager_example 
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include

)

target_link_libraries(card_manager_example PRIVATE
    NfcCpp
    etl::etl
)

# Set output directory
set_target_properties(card_manager_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)


# On Windows, copy any required DLLs
if(WIN32)
    # Add any Windows-specific settings here
endif()
