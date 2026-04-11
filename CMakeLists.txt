cmake_minimum_required(VERSION 3.16)
project(velour C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(wallrift-daemon
    src/daemon.c
    src/utils.c
    src/zwlr-layer-shell-protocol.c
    src/xdg-shell-protocol.c
)

target_link_libraries(wallrift-daemon
    wayland-client
    wayland-egl
    EGL
    GLESv2
    m
)

add_executable(wallrift
    src/cli.c
    src/utils.c
    src/zwlr-layer-shell-protocol.c
    src/xdg-shell-protocol.c
)

target_link_libraries(wallrift
    wayland-client
    wayland-egl
    EGL
    GLESv2
    m
)
