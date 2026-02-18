# desfire_value_operations - GetValue/Credit/Debit/LimitedCredit example

add_executable(desfire_value_operations_example
    main.cpp
)

target_include_directories(desfire_value_operations_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

target_link_libraries(desfire_value_operations_example
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
)

set_target_properties(desfire_value_operations_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/$<CONFIG>"
)

