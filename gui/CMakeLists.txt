set(
    SRC
    "main.cpp"
    "window.cpp"
)

set(NAME
    "e1280-gui"
    
)

set(CMAKE_CXX_STANDARD 20)

add_executable(${NAME} ${SRC})

set(FLTK_BUILD_TEST OFF CACHE BOOL " " FORCE)
include(FetchContent)
FetchContent_Declare(
    FLTK
    GIT_REPOSITORY https://github.com/fltk/fltk.git
    GIT_TAG release-1.3.8
)
FetchContent_MakeAvailable(FLTK)

target_include_directories(${NAME} PRIVATE ${fltk_BINARY_DIR} ${fltk_SOURCE_DIR})
target_link_libraries(${NAME} PRIVATE fltk e1280-core)
