
set(
    SRC
    "main.cpp"
)

set(NAME "e1280-cli")
set(CMAKE_CXX_STANDARD 20)
add_executable(${NAME} ${SRC})

target_link_libraries(${NAME} PRIVATE e1280-core)
