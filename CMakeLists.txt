cmake_minimum_required(VERSION 3.14)

project(
    "e1280"
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable GCC extensions
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

add_subdirectory(core)

if(FRONTEND STREQUAL  "cli")
    add_subdirectory(cli)
elseif(FRONTEND STREQUAL "gui")
    message("gui frontend not yet implemented")
else()

endif()
