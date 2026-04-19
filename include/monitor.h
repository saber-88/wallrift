#pragma once

#include <wayland-client.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
typedef struct APP APP;

typedef struct Monitor {
    struct APP *app;
    struct wl_output *output;

    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;

    struct wl_egl_window *egl_window;
    EGLSurface egl_surface;

    int width, height;
    int scale;

    int configured;
    int pending_frame;

    struct wl_callback *frame_cb;

    int pointer_inside;
    double cursor_x;
    double target_cursor;

    GLuint textureId;
    int img_w, img_h;
    uint32_t global_name;
    char wallpath[5000];

} Monitor;
