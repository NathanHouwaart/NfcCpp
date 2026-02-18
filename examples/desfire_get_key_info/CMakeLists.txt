# desfire_get_key_info - GetKeySettings + GetKeyVersion example

add_executable(desfire_get_key_info_example
    main.cpp
)

target_include_directories(desfire_get_key_info_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

target_link_libraries(desfire_get_key_info_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

set_target_properties(desfire_get_key_info_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)

