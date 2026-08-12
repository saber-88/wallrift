#pragma once

#include "monitor.h"
#include "zwlr-layer-shell-unstable-v1-protocol.h"
#include "ext-idle-notify-v1-client-protocol.h"
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
  struct ext_idle_notifier_v1 *notifier;  
  struct ext_idle_notification_v1 *notification;  
  double last_cursor_x;
  double last_cursor_y;
  int wayland_fd;
  int cursor_moved;

} WLGlobal;

Monitor* find_monitor_by_name(APP *app, const char* name);
Monitor * find_monitor_by_surface(APP* app, struct wl_surface *wl_surface);
Monitor * find_monitor_by_layer_surface(APP* app, struct zwlr_layer_surface_v1 *ls);
void setup_wayland(APP *app);
void setup_cursor(APP *app);
void setup_egl_global(APP *app);
void setup_surface(APP *app, Monitor* m);
void setup_egl(APP *app, Monitor* m);
void request_frame(Monitor *m);


