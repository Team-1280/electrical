set(
    SRC
    "lib.cpp"
    "unit.cpp"
    "geom.cpp"
    "util/log.cpp"
    "component.cpp"
)

set(
    NAME
    "e1280-core"
)

set(CMAKE_CXX_STANDARD 20)

add_library(${NAME} ${SRC})

target_include_directories(${NAME} PUBLIC ./)
target_link_libraries(${NAME} PUBLIC nlohmann_json::nlohmann_json fmt::fmt)
