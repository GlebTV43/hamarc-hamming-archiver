add_library(hamarc_lib STATIC
    src/archiver.cpp
    src/argparser.cpp
    src/coder.cpp
    src/hamming.cpp
)

target_include_directories(hamarc_lib
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(hamarc_lib PUBLIC cxx_std_20)

install(TARGETS hamarc_lib
    EXPORT hamarc-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT hamarc-targets
    FILE hamarc-config.cmake
    NAMESPACE hamarc::
    DESTINATION lib/cmake/hamarc
)