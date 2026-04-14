cmake_minimum_required(VERSION 3.16)
project(wallrift C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(wallrift-daemon
    src/daemon.c
    src/file.c
    src/wayland.c
    src/gl.c
    src/zwlr-layer-shell-protocol.c
    src/xdg-shell-protocol.c
)

target_link_libraries(wallrift-daemon
    wayland-client
    wayland-egl
    wayland-cursor
    EGL
    GLESv2
    m
)

add_executable(wallrift
    src/cli.c
    src/file.c
    src/wayland.c
    src/gl.c
    src/zwlr-layer-shell-protocol.c
    src/xdg-shell-protocol.c
)

target_link_libraries(wallrift
    wayland-client
    wayland-egl
    wayland-cursor
    EGL
    GLESv2
    m
)
