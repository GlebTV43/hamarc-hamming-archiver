add_library(hamarc_lib
    argparser.cpp
    argparser.h
    hamming.cpp  
    hamming.h
    coder.cpp
    coder.h
    archiver.cpp
    archiver.h
)

target_include_directories(hamarc_lib
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(hamarc_lib PRIVATE cxx_std_23)