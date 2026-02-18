# desfire_create_std_data_file - Create standard data file in selected application

add_executable(desfire_create_std_data_file_example
    main.cpp
)

target_include_directories(desfire_create_std_data_file_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

target_link_libraries(desfire_create_std_data_file_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

set_target_properties(desfire_create_std_data_file_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)

