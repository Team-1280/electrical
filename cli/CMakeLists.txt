
set(
    SRC
    "main.cpp"
    "args.cpp"
)

set(NAME "e1280_cli")
set(CMAKE_CXX_STANDARD 20)
add_executable(${NAME} ${SRC})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/buildopts-${NAME}.h.in" "generated/buildopts-${NAME}.h")
target_include_directories(${NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")


target_link_libraries(${NAME} PRIVATE e1280)
