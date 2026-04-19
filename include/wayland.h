#pragma once

#include "monitor.h"
#include "zwlr-layer-shell-unstable-v1-protocol.h"
#include <EGL/egl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-util.h>
#include <wayland-cursor.h>
#include <GL/gl.h>

typedef struct APP APP;
typedef struct WLGlobal{

  struct wl_display *display;
  struct wl_compositor *compositor;
  struct wl_registry *registry;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct wl_seat *seat;
  struct wl_pointer *pointer;
  struct wl_cursor *cursor;
  struct wl_cursor_theme *cursor_theme;
  struct wl_surface *cursor_surface;
  struct wl_shm* shm;

  int wayland_fd;

} WLGlobal;

Monitor * find_monitor_by_surface(APP* app, struct wl_surface *wl_surface);
Monitor * find_monitor_by_layer_surface(APP* app, struct zwlr_layer_surface_v1 *ls);

static void registry_handler(void *data, struct wl_registry *registry,
                             uint32_t id, const char *interface,
                             uint32_t version);



static const struct wl_callback_listener frame_listener;

static void registry_remove(void *data, struct wl_registry *registry,
                            uint32_t id);


static void layer_configure(void* data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t widht, uint32_t height);

static void layer_closed(void *data, struct zwlr_layer_surface_v1 *surface);


// pointer stuff
static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t sx, wl_fixed_t sy);

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface); 

static void pointer_button(void *data, struct wl_pointer *pointer,
                           uint32_t serial, uint32_t time,
                           uint32_t button, uint32_t state);

static void pointer_axis(void *data, struct wl_pointer *pointer,
                         uint32_t time, uint32_t axis, wl_fixed_t value);

//frame callback stuff

static void frame_done(void *data, struct wl_callback *cb, uint32_t time);

void setupWayland(APP *app);
void setupCursor(APP *app);
void setupEGLGlobal(APP *app);
void setupSurface(APP *app, Monitor* m);
void setupEGL(APP *app, Monitor* m);


