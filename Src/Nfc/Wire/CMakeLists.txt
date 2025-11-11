# Nfc Wire module

add_library(NfcCpp_Nfc_Wire OBJECT)

target_sources(NfcCpp_Nfc_Wire
    PRIVATE
        NativeWire.cpp
        IsoWire.cpp
)

target_include_directories(NfcCpp_Nfc_Wire
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
)

target_link_libraries(NfcCpp_Nfc_Wire
    PRIVATE
        etl::etl
)
