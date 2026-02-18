# Desfire module

add_library(NfcCpp_Nfc_Desfire OBJECT
    DesfireCard.cpp
    PlainPipe.cpp
    MacPipe.cpp
    EncPipe.cpp
    Commands/AuthenticateCommand.cpp
    Commands/GetVersionCommand.cpp
    Commands/FormatPiccCommand.cpp
    Commands/FreeMemoryCommand.cpp
    Commands/GetApplicationIdsCommand.cpp
    Commands/GetFileIdsCommand.cpp
    Commands/GetFileSettingsCommand.cpp
    Commands/ChangeFileSettingsCommand.cpp
    Commands/GetKeySettingsCommand.cpp
    Commands/GetKeyVersionCommand.cpp
    Commands/GetCardUidCommand.cpp
    Commands/ChangeKeyCommand.cpp
    Commands/ChangeKeySettingsCommand.cpp
    Commands/SetConfigurationCommand.cpp
    Commands/CreateApplicationCommand.cpp
    Commands/DeleteApplicationCommand.cpp
    Commands/CreateStdDataFileCommand.cpp
    Commands/CreateBackupDataFileCommand.cpp
    Commands/CreateLinearRecordFileCommand.cpp
    Commands/CreateCyclicRecordFileCommand.cpp
    Commands/CreateValueFileCommand.cpp
    Commands/DeleteFileCommand.cpp
    Commands/ClearRecordFileCommand.cpp
    Commands/GetValueCommand.cpp
    Commands/CreditCommand.cpp
    Commands/DebitCommand.cpp
    Commands/LimitedCreditCommand.cpp
    Commands/CommitTransactionCommand.cpp
    Commands/ReadDataCommand.cpp
    Commands/WriteDataCommand.cpp
    Commands/ReadRecordsCommand.cpp
    Commands/WriteRecordCommand.cpp
    Commands/SelectApplicationCommand.cpp
)

target_include_directories(NfcCpp_Nfc_Desfire PUBLIC
    ${CMAKE_SOURCE_DIR}/Include
    ${CMAKE_SOURCE_DIR}/external/etl/include
    ${CMAKE_SOURCE_DIR}/external/cppDes/include
    ${CMAKE_SOURCE_DIR}/external/tiny-aes
)

target_link_libraries(NfcCpp_Nfc_Desfire PUBLIC
    etl::etl
    tiny-aes
    libcppdes
)
