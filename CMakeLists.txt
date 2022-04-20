cmake_minimum_required(VERSION 3.14)

project(
    "e1280"
    VERSION 0.1.0
    LANGUAGES CXX
)

if(MSVC)
    set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "/std:c++20")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable GCC extensions
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TYPES Release RelWithDebInfo MinSizeRel Debug)

include(FetchContent)

FetchContent_Declare(
    nlohmann-json URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann-json)

if("${CMAKE_BUILD_TYPE}" STREQUAL "${CMAKE_DEFAULT_BUILD_TYPE}")
    message("No build type given, building in debug mode")
    set(CMAKE_BUILD_TYPE "Debug")
elseif(NOT "${CMAKE_BUILD_TYPE}" IN_LIST BUILD_TYPES)
    message(FATAL_ERROR "Invalid build type ${CMAKE_BUILD_TYPE}, available build types are: ${BUILD_TYPES}")
endif()
configure_file("build/buildopts.h.in" "generated/buildopts.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/generated")

add_subdirectory(core)

if(FRONTEND STREQUAL  "cli")
    add_subdirectory(cli)
elseif(FRONTEND STREQUAL "gui")
    message("gui frontend not yet implemented")
else()

endif()
