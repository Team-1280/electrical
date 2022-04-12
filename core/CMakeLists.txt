set(
    SRC
    "lib.cpp"
)

set(
    NAME
    "e1280-core"
)

add_library(${NAME} ${SRC})
target_include_directories(${NAME} PUBLIC ./)
target_compile_features(${NAME} PUBLIC cxx_std_11)
