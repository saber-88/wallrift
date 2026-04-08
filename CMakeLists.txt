cmake_minimum_required(VERSION 3.16)
project(velour C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(velour
    src/main.c
    src/zwlr-layer-shell-protocol.c
    src/xdg-shell-protocol.c
)

target_link_libraries(velour
    wayland-client
    wayland-egl
    EGL
    GLESv2
    m
)
