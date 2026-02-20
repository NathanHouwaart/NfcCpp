# Unit Tests

# Serial Bus Tests
add_executable(test_serial_bus
    SerialBusTests.cpp
)

target_link_libraries(test_serial_bus
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_serial_bus
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/Src
)

# Register test with CTest
add_test(NAME SerialBusTests COMMAND test_serial_bus)

# Error Handling Tests
add_executable(test_error
    ErrorTests.cpp
)

target_link_libraries(test_error
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_error
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME ErrorTests COMMAND test_error)

# PN532 Driver Tests
add_executable(test_pn532
    Pn532Tests.cpp
)

target_link_libraries(test_pn532
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_pn532
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME Pn532Tests COMMAND test_pn532)

# Timing Utilities Tests
add_executable(test_timing
    Utils/TimingTests.cpp
)

target_link_libraries(test_timing
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_timing
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME TimingTests COMMAND test_timing)

# DESFire ChangeKey command tests
add_executable(test_desfire_change_key
    DesfireChangeKeyCommandTests.cpp
)

target_link_libraries(test_desfire_change_key
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_desfire_change_key
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME DesfireChangeKeyCommandTests COMMAND test_desfire_change_key)

# DESFire Authenticate command tests
add_executable(test_desfire_authenticate
    DesfireAuthenticateCommandTests.cpp
)

target_link_libraries(test_desfire_authenticate
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        tiny-aes
        gtest
        gtest_main
)

target_include_directories(test_desfire_authenticate
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/external/cppDes/include
        ${CMAKE_SOURCE_DIR}/external/tiny-aes
)

add_test(NAME DesfireAuthenticateCommandTests COMMAND test_desfire_authenticate)

# DESFire CreateApplication command tests
add_executable(test_desfire_create_application
    DesfireCreateApplicationCommandTests.cpp
)

target_link_libraries(test_desfire_create_application
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_desfire_create_application
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME DesfireCreateApplicationCommandTests COMMAND test_desfire_create_application)

# DESFire DeleteApplication command tests
add_executable(test_desfire_delete_application
    DesfireDeleteApplicationCommandTests.cpp
)

target_link_libraries(test_desfire_delete_application
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_desfire_delete_application
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME DesfireDeleteApplicationCommandTests COMMAND test_desfire_delete_application)

# DESFire GetFileIDs command tests
add_executable(test_desfire_get_file_ids
    DesfireGetFileIdsCommandTests.cpp
)

target_link_libraries(test_desfire_get_file_ids
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_desfire_get_file_ids
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME DesfireGetFileIdsCommandTests COMMAND test_desfire_get_file_ids)

# DESFire ReadData + WriteData command tests
add_executable(test_desfire_read_write_data
    DesfireReadWriteDataCommandTests.cpp
)

target_link_libraries(test_desfire_read_write_data
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_desfire_read_write_data
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME DesfireReadWriteDataCommandTests COMMAND test_desfire_read_write_data)

# DESFire SecureMessagingPolicy tests
add_executable(test_desfire_secure_messaging_policy
    DesfireSecureMessagingPolicyTests.cpp
)

target_link_libraries(test_desfire_secure_messaging_policy
    PRIVATE
        NfcCpp::NfcCpp
        etl::etl
        gtest
        gtest_main
)

target_include_directories(test_desfire_secure_messaging_policy
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Include
)

add_test(NAME DesfireSecureMessagingPolicyTests COMMAND test_desfire_secure_messaging_policy)
