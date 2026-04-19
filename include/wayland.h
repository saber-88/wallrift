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
void setupWayland(APP *app);
void setupCursor(APP *app);
void setupEGLGlobal(APP *app);
void setupSurface(APP *app, Monitor* m);
void setupEGL(APP *app, Monitor* m);


