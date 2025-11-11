# Nfc Card module

add_library(NfcCpp_Nfc_Card OBJECT)

target_sources(NfcCpp_Nfc_Card
    PRIVATE
        CardManager.cpp
        CardSession.cpp
        ReaderCapabilities.cpp
)

target_include_directories(NfcCpp_Nfc_Card
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
)

target_link_libraries(NfcCpp_Nfc_Card
    PRIVATE
        etl::etl
)
