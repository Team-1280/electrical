set(
    SRC
    "main.cpp"
)

set(NAME
    "e1280_gui"
)

set(CMAKE_CXX_STANDARD 20)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(FLTK REQUIRED)

add_executable(${NAME} ${SRC})

target_include_directories(${NAME} PUBLIC ${FLTK_INCLUDE_DIR})
target_link_libraries(${NAME} PRIVATE ${FLTK_LIBRARIES} e1280)
