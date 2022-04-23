set(
    SRC
    "main.cpp"
)

set(NAME
    "e1280-gui"
    
)

set(CMAKE_CXX_STANDARD 20)

add_executable(${NAME} ${SRC})
target_link_libraries(${NAME} PUBLIC fltk::fltk)
