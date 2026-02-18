# desfire_auth_changekey - Demonstrates DESFire select app, authenticate, and change key

add_executable(desfire_auth_changekey_example
    main.cpp
)

target_include_directories(desfire_auth_changekey_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

target_link_libraries(desfire_auth_changekey_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

# Set output directory
set_target_properties(desfire_auth_changekey_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)
