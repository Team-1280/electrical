set(
    SRC
    "main.cpp"
    "window.cpp"
)

set(NAME
    "e1280-gui"
    
)

set(CMAKE_CXX_STANDARD 20)

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)

set(GRES_OUT "${CMAKE_CURRENT_BINARY_DIR}/resources.c")
set(GRES_IN "${CMAKE_CURRENT_SOURCE_DIR}/res/resources.xml")


add_executable(${NAME} ${SRC} ${GRES_OUT})
add_custom_command(
    COMMENT "Generating resources"
    OUTPUT ${GRES_OUT}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res"
    COMMAND ${GLIB_COMPILE_RESOURCES}
    ARGS
        --generate-source
        --target=${GRES_OUT}
        ${GRES_IN}
    VERBATIM
    MAIN_DEPENDENCY ${GRES_IN}
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/res/img/new-component.png"
)

add_custom_target(resources DEPENDS ${GRES_OUT})
set_source_files_properties(
    ${GRES_OUT}
    PROPERTIES GENERATED TRUE
)
add_dependencies(${NAME} resources)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

target_link_directories(${NAME} PRIVATE ${GTKMM_LIBRARY_DIRS})
target_include_directories(${NAME} PRIVATE ${GTKMM_INCLUDE_DIRS})
target_link_libraries(${NAME} PRIVATE ${GTKMM_LIBRARIES} e1280-core)
