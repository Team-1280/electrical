set(
    SRC
    "lib.cpp"
    "util/log.cpp"
)

set(
    NAME
    "e1280-core"
)

set(CMAKE_CXX_STANDARD 17)

add_library(${NAME} ${SRC})
target_include_directories(${NAME} PUBLIC ./)
target_link_libraries(${NAME} PUBLIC nlohmann_json::nlohmann_json)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${NAME} DISABLE_TRACE)
endif()
