set(
    SRC
    "lib.cpp"
    "unit.cpp"
    "geom.cpp"
    "util/log.cpp"
    "component.cpp"
    "resource.cpp"
    "wire.cpp"
    "ser/store.cpp"
)

set(
    NAME
    "e1280-core"
)

set(CMAKE_CXX_STANDARD 20)

add_library(${NAME} ${SRC})
include(FetchContent)
FetchContent_Declare(
    stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG v1.2.2
)
FetchContent_GetProperties(stduuid)
if(NOT stduuid_POPULATED)
    FetchContent_Populate(stduuid)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${stduuid_SOURCE_DIR}/cmake")
    set(UUID_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(UUID_SYSTEM_GENERATOR ON CACHE BOOL "" FORCE)
    add_subdirectory(${stduuid_SOURCE_DIR} ${stduuid_BINARY_DIR})
endif()

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

target_compile_options(${NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
target_compile_definitions(${NAME} PRIVATE UUID_SYSTEM_GENERATOR)
target_include_directories(${NAME} PUBLIC ./)
target_link_libraries(${NAME} PUBLIC nlohmann_json::nlohmann_json fmt::fmt stduuid)
