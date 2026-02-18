# desfire_authenticate - Authenticate to DESFire PICC/application

add_executable(desfire_authenticate_example
    main.cpp
)

target_include_directories(desfire_authenticate_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

target_link_libraries(desfire_authenticate_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

set_target_properties(desfire_authenticate_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)

