# Nfc module

add_subdirectory(Card)
add_subdirectory(Wire)
add_subdirectory(Desfire)

add_library(NfcCpp_Nfc OBJECT)

target_sources(NfcCpp_Nfc
    PRIVATE
        $<TARGET_OBJECTS:NfcCpp_Nfc_Card>
        $<TARGET_OBJECTS:NfcCpp_Nfc_Wire>
        $<TARGET_OBJECTS:NfcCpp_Nfc_Desfire>
)

target_include_directories(NfcCpp_Nfc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
)

target_link_libraries(NfcCpp_Nfc
    PUBLIC
        NfcCpp_Nfc_Card
        NfcCpp_Nfc_Wire
        NfcCpp_Nfc_Desfire
)
