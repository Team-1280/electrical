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

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

target_link_directories(${NAME} PRIVATE ${GTKMM_LIBRARY_DIRS})
target_include_directories(${NAME} PRIVATE ${GTKMM_INCLUDE_DIRS})
target_link_libraries(${NAME} PRIVATE ${GTKMM_LIBRARIES} e1280-core)
