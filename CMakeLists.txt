cmake_minimum_required(VERSION 3.16)
project(wallrift C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# daemon
add_executable(wallrift-daemon
    src/daemon.c
    src/file.c
    src/wayland.c
    src/gl.c
    src/zwlr-layer-shell-protocol.c
    src/xdg-shell-protocol.c
    src/ext-idle-notify-v1-protocol.c
)

target_include_directories(wallrift-daemon PRIVATE include)

target_link_libraries(wallrift-daemon
    wayland-client
    wayland-egl
    wayland-cursor
    EGL
    GLESv2
    m
)

# warnings — always on
target_compile_options(wallrift-daemon PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wpointer-arith
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wnull-dereference
    -Wformat=2
    -fstack-protector-strong
)

# debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(wallrift-daemon PRIVATE
        -fsanitize=address,undefined
        -g
        -O0
        -Werror
    )
    target_link_options(wallrift-daemon PRIVATE
        -fsanitize=address,undefined
    )
    target_compile_definitions(wallrift-daemon PRIVATE
        LOG_LEVEL=0
    )
endif()

# release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(wallrift-daemon PRIVATE
        -O2
    )
    target_compile_definitions(wallrift-daemon PRIVATE
        LOG_LEVEL=1
        NDEBUG
    )
endif()

# CLI — no GPU deps needed
add_executable(wallrift
    src/cli.c
)
target_include_directories(wallrift PRIVATE include)
