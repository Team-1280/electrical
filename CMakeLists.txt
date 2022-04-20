cmake_minimum_required(VERSION 3.14)

project(
    "e1280"
    VERSION 0.1.0
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable GCC extensions
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    nlohmann-json URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann-json)

if("${CMAKE_BUILD_TYPE}" STREQUAL "${CMAKE_DEFAULT_BUILD_TYPE}")
    message("No build type given, building in debug mode")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    #string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)
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
