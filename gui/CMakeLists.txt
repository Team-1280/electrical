set(
    SRC
    "main.cpp"
)

set(NAME
    "e1280-gui"
    
)

set(CMAKE_CXX_STANDARD 20)

add_executable(${NAME} ${SRC})

set(FLTK_BUILD_TEST OFF CACHE BOOL " " FORCE)
include(FetchContent)
FetchContent_Declare(
    fltk
    GIT_REPOSITORY https://github.com/fltk/fltk.git
    GIT_TAG master
)
FetchContent_MakeAvailable(fltk)


target_link_libraries(${NAME} PUBLIC fltk e1280-core)
