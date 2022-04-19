cmake_minimum_required(VERSION 3.23)

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

add_subdirectory(core)

if(FRONTEND STREQUAL  "cli")
    add_subdirectory(cli)
elseif(FRONTEND STREQUAL "gui")
    message("gui frontend not yet implemented")
else()

endif()
