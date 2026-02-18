# desfire_read_write_data - ReadData + WriteData example

add_executable(desfire_read_write_data_example
    main.cpp
)

target_include_directories(desfire_read_write_data_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

target_link_libraries(desfire_read_write_data_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

set_target_properties(desfire_read_write_data_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)

