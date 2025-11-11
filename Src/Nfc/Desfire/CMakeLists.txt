# Desfire module

add_library(NfcCpp_Nfc_Desfire OBJECT
    DesfireCard.cpp
    PlainPipe.cpp
    MacPipe.cpp
    EncPipe.cpp
    Commands/AuthenticateCommand.cpp
    Commands/ChangeKeyCommand.cpp
)

target_include_directories(NfcCpp_Nfc_Desfire PUBLIC
    ${CMAKE_SOURCE_DIR}/Include
    ${CMAKE_SOURCE_DIR}/external/etl/include
)

target_link_libraries(NfcCpp_Nfc_Desfire PUBLIC
    etl
)
