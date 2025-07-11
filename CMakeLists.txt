cmake_minimum_required(VERSION 3.31)

project(init LANGUAGES C)

include_directories(SYSTEM
        ${SDK}/include/
        ${SDK}/linux/tools/include/nolibc
)

add_executable(init
        Sources/drm.c
        Sources/input.c
        Sources/main.c
        Sources/shell.c
        Sources/snake.c
        Sources/utils.c
        Sources/vec.c
        Sources/vt.c
)
set_property(
        TARGET init
        PROPERTY C_STANDARD 23
)
