add_library(hamarc_lib STATIC
    src/archiver.cpp
    src/argparser.cpp
    src/coder.cpp
    src/hamming.cpp
)

target_include_directories(hamarc_lib
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(hamarc_lib PUBLIC cxx_std_20)