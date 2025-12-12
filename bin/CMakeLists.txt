add_executable(hamarc main.cpp)

target_link_libraries(hamarc PRIVATE hamarc_lib)

install(TARGETS hamarc DESTINATION bin)