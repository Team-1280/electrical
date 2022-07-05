set(
    SRC
    "lib.cpp"
    "unit.cpp"
    "geom.cpp"
    "util/log.cpp"
    "component.cpp"
    "wire.cpp"
    "ser/store.cpp"
    "data.cpp"
    "currency.cpp"
)

set(
    NAME
    "e1280"
)
set(TESTNAME "${NAME}_test")

set(CMAKE_CXX_STANDARD 20)

add_library(${NAME} STATIC ${SRC})

include(FetchContent)

option(BUILD_CORE_DOCS "Build doxygen documentation for the core library" OFF)
find_package(Doxygen)
if(DOXYGEN_FOUND AND BUILD_CORE_DOCS) 
    configure_file(${CMAKE_BINARY_DIR}/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(build_core_docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating core library docs"
        VERBATIM
    )
elseif(BUILD_CORE_DOCS)
    message("Install doxygen to build documentation")
endif()
include(CTest)
enable_testing()
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
)
FetchContent_MakeAvailable(doctest)
list(APPEND CMAKE_MODULE_PATH "${doctest_SOURCE_DIR}/scripts/cmake")
include(doctest)
include("${CMAKE_CURRENT_SOURCE_DIR}/doctest_force_static_link_lib_in_target.cmake")
include_directories("${doctest_SOURCE_DIR}/doctest")

target_compile_options(${NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_include_directories(${NAME} PUBLIC ./)
target_link_libraries(${NAME} PUBLIC nlohmann_json::nlohmann_json fmt::fmt doctest)

#doctest_discover_tests(${NAME})
doctest_make_exe_for_static_lib(${TESTNAME} ${NAME})
