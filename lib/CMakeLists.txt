set(
    SRC
    "lib.cpp"
    "unit.cpp"
    "geom.cpp"
    "util/log.cpp"
    "util/freelist.cpp"
    "util/optional.cpp"
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
set(OBJNAME "${NAME}_obj")
set(CMAKE_CXX_STANDARD 20)

add_library(${OBJNAME} OBJECT ${SRC})

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

if(BUILD_RELEASE)
    message("Build type is release, disabling tests")
    set(DOCTEST_CONFIG_DISABLE ON)
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

target_compile_options(${OBJNAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_include_directories(${OBJNAME} PUBLIC ./ "${doctest_SOURCE_DIR}/doctest")
target_link_libraries(${OBJNAME} PUBLIC nlohmann_json::nlohmann_json fmt::fmt doctest)

add_executable(${TESTNAME} "test.cpp" $<TARGET_OBJECTS:${OBJNAME}>)
target_link_libraries(${TESTNAME} PUBLIC ${OBJNAME})
doctest_discover_tests(${TESTNAME})

add_library(${NAME} STATIC)
target_link_libraries(${NAME} ${OBJNAME})

if(NOT BUILD_RELEASE)
    message("Build type is debug, tests will be run every build")
    add_custom_command(
        TARGET ${TESTNAME}
        POST_BUILD
        COMMAND CTEST_OUTPUT_ON_FAILURE=1 ctest
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${TESTNAME}>
        VERBATIM
        USES_TERMINAL
        COMMENT "Running tests..."
     )
endif()
