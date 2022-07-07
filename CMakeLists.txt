cmake_minimum_required(VERSION 3.4)

project(
    "e1280"
    VERSION 0.1.0
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(MSVC)
    set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "/std:c++20")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable GCC extensions
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TYPES Release RelWithDebInfo MinSizeRel Debug)

if("${CMAKE_BUILD_TYPE}" STREQUAL "${CMAKE_DEFAULT_BUILD_TYPE}")
    message("No build type given, building in debug mode")
    set(CMAKE_BUILD_TYPE "Debug")
elseif(NOT "${CMAKE_BUILD_TYPE}" IN_LIST BUILD_TYPES)
    message(FATAL_ERROR "Invalid build type ${CMAKE_BUILD_TYPE}, available build types are: ${BUILD_TYPES}")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL Debug) 
    set(BUILD_RELEASE)
endif()

configure_file("${CMAKE_SOURCE_DIR}/build/buildopts.h.in" "generated/buildopts.h")
include_directories("${CMAKE_BINARY_DIR}/generated")
include(FetchContent)

set(JSON_ImplicitConversions CACHE BOOL OFF)
set(JSON_Diagnostics CACHE BOOL ON "")
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_compile_definitions(JSON_DIAGNOSTICS=1)
FetchContent_Declare(
    nlohmann-json URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann-json)

FetchContent_Declare(
    fmt 
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

add_subdirectory(lib)

set(FRONTEND_TYPES cli gui)
if(NOT "${FRONTEND}" IN_LIST FRONTEND_TYPES)
    message("No frontend given, building gui")
    set(FRONTEND "gui")
endif()
if(FRONTEND STREQUAL  "cli")
    add_subdirectory(cli)
elseif(FRONTEND STREQUAL "gui")
    add_subdirectory(gui)
else()
    message(FATAL "Invalid frontend option ${FRONTEND}")
endif()
