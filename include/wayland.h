#pragma once

#include "zwlr-layer-shell-unstable-v1-protocol.h"
#include <EGL/egl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <string.h>
#include <wayland-util.h>

typedef struct {

  struct wl_display *display;
  struct wl_compositor *compositor;
  struct wl_registry *registry;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct wl_surface *surface;
  struct zwlr_layer_surface_v1 *layer_surface;
  struct wl_seat *seat;
  struct wl_pointer *pointer;
  double mouse_x;
  double target_cursor;
  double cursor_x;

  struct wl_egl_window *egl_window;

  EGLDisplay egl_display;
  EGLContext egl_context;
  EGLSurface egl_surface;
  int configured, width, height;
} WL;

static void registry_handler(void *data, struct wl_registry *registry,
                             uint32_t id, const char *interface,
                             uint32_t version) 
{
  WL *wl = (WL*)data;
  if (!strcmp(interface, "wl_compositor")) {
    wl->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
  }
  else if (!strcmp(interface, "zwlr_layer_shell_v1")) {
    wl->layer_shell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
  }
  else if (!strcmp(interface, "wl_seat")) {
    wl->seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
  }
}

static void registry_remove(void *data, struct wl_registry *registry,
                            uint32_t id) {};

// wl registry listener
static const struct wl_registry_listener registry_listener = {registry_handler,
                                                              registry_remove};

static void layer_configure(void* data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t widht, uint32_t height){
  WL *wl = (WL*)data;
  wl->width = widht;
  wl->height = height;
  wl->configured = 1;
  zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_closed(void *data, struct zwlr_layer_surface_v1 *surface){
  exit(0);
}

// layer-shell surface listener 
static const struct zwlr_layer_surface_v1_listener layer_listener= {
  layer_configure,
  layer_closed
};


// pointer stuff
static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy){
  WL *wl = (WL*)data;
  wl->mouse_x = wl_fixed_to_double(sx);

  float normalized = wl->mouse_x / wl->width;
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  wl->target_cursor = normalized;
}
static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t sx, wl_fixed_t sy) {}

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface) {}

static void pointer_button(void *data, struct wl_pointer *pointer,
                           uint32_t serial, uint32_t time,
                           uint32_t button, uint32_t state) {}

static void pointer_axis(void *data, struct wl_pointer *pointer,
                         uint32_t time, uint32_t axis, wl_fixed_t value) {}

static const struct wl_pointer_listener pointer_listener = {
    pointer_enter,
    pointer_leave,
    pointer_motion,
    pointer_button,
    pointer_axis,
};



