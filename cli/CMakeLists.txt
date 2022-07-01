
set(
    SRC
    "main.cpp"
    "args.cpp"
    "cmd.cpp"
)

set(NAME "e1280_cli")
set(CMAKE_CXX_STANDARD 20)
add_executable(${NAME} ${SRC})


target_link_libraries(${NAME} PRIVATE e1280)
