# Utils Library

add_library(NfcCpp_Utils OBJECT
    DesfireCrypto.cpp
    TimingEmbedded.cpp
)

target_include_directories(NfcCpp_Utils
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
)

target_link_libraries(NfcCpp_Utils
    PUBLIC
        etl::etl
    PRIVATE
        libcppdes
)

