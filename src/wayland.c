#include "wayland.h"
#include "gl.h"
#include "app.h"
#include "monitor.h"
#include "zwlr-layer-shell-unstable-v1-protocol.h"
#include <EGL/egl.h>
#include <GL/gl.h>
#include <inttypes.h>
#include <stdio.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-egl-core.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>

Monitor* find_monitor_by_surface(APP *app, struct wl_surface* surface){
  for (int i = 0 ; i < app->monitor_count; i++) {
    if (app->monitors[i].surface == surface) {
      return &app->monitors[i];
    }
  }
  return NULL;
}

Monitor * find_monitor_by_layer_surface(APP* app, struct zwlr_layer_surface_v1 *ls){
  for (int i = 0 ; i < app->monitor_count; i++) {
    if (app->monitors[i].layer_surface == ls) {
      return &app->monitors[i];
    }
  }
  return NULL;
}

static const struct wl_callback_listener frame_listener = {
  .done = frame_done 
};


static void registry_handler(void *data, struct wl_registry *registry,
                             uint32_t id, const char *interface,
                             uint32_t version) 
{
  APP *app = (APP*)data;
  if (!strcmp(interface, "wl_compositor")) {
    app->wl.compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
  }
  else if (!strcmp(interface, "zwlr_layer_shell_v1")) {
    app->wl.layer_shell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
  }
  else if (!strcmp(interface, "wl_seat")) {
    app->wl.seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
  }
  else if (!strcmp(interface, "wl_shm")) {
    app->wl.shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
  }
  else if (!strcmp(interface, "wl_output")) {
    Monitor *m = &app->monitors[app->monitor_count++];
    memset(m, 0, sizeof(Monitor));
    m->output = wl_registry_bind(registry, id, &wl_output_interface, 1);
    m->global_name = id;
    m->app = app;
    m->cursor_x = 0.05f;
    if (app->monitor_count == 1) {
      app->active_monitor = m;
    }

  }
}

static void registry_remove(void *data, struct wl_registry *registry,
                            uint32_t id) {
  APP *app = (APP *)data;
  for (int i = 0; i < app->monitor_count; i++) {
    if (app->monitors[i].global_name == id) {
        Monitor *m = &app->monitors[i];
        if (app->active_monitor == m) {
          app->active_monitor =  NULL;     

          for (int j = 0; j < app->monitor_count; j++) {
            if (j != i) {
              app->active_monitor = &app->monitors[j];
              break;
            }
          }
        }
        if (m->frame_cb) {
          wl_callback_destroy(m->frame_cb);
          m->frame_cb = NULL;
        }
        if (m->egl_surface != EGL_NO_SURFACE) {
          eglDestroySurface(app->egl.egl_display, m->egl_surface);
          m->egl_surface = EGL_NO_SURFACE;
        }
        if (m->layer_surface) {
          zwlr_layer_surface_v1_destroy(m->layer_surface);
          m->layer_surface = NULL;
        }
        if (m->surface) {
          wl_surface_destroy(m->surface);
          m->surface = NULL;
        }
        if (m->output) {
          wl_output_destroy(m->output);
          m->output = NULL;
        }
        if (m->textureId != 0) {
          glDeleteTextures(1, &m->textureId);
        }

        for (int j = i; j < app->monitor_count - 1; j++) {
          app->monitors[j] = app->monitors[j+1];
        }
        app->monitor_count--;
        
        break;
    }
  }
};

// wl registry listener
static const struct wl_registry_listener registry_listener = {registry_handler,
                                                              registry_remove};

static void layer_configure(void* data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t width, uint32_t height){
  Monitor *m = (Monitor*)data;

  m->width = width;
  m->height = height;
  m->configured = 1;
  zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_closed(void *data, struct zwlr_layer_surface_v1 *surface){
}

// layer-shell surface listener 
static const struct zwlr_layer_surface_v1_listener layer_listener= {
  layer_configure,
  layer_closed
};


// pointer stuff
static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy){
  APP *app = (APP *)data;
  if (!app->active_monitor) {
    printf("[ERR][MONITOR]: No acitve monitor found\n");
    return;
  }
  app->active_monitor->target_cursor = wl_fixed_to_double(sx);

  float normalized = app->active_monitor->target_cursor / app->active_monitor->width;
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  app->active_monitor->target_cursor = normalized;
}
static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t sx, wl_fixed_t sy) {

  APP *app = (APP *)data;
  Monitor *m = find_monitor_by_surface(app, surface);
  if (!m) {
    printf("[ERR][MONITOR]: No monitor found by surface\n");
    return;
  }
  app->active_monitor = m;
  m->pointer_inside = 1;

  if (!m->pending_frame) {
      m->frame_cb = wl_surface_frame(m->surface);
      wl_callback_add_listener(m->frame_cb, &frame_listener, m);
      m->pending_frame = 1;
      wl_surface_commit(m->surface);
  }

  m->target_cursor = wl_fixed_to_double(sx);

  float normalized = m->target_cursor / m->width;
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  m->target_cursor = normalized;

  if (app->wl.cursor && app->wl.cursor_surface) {
    struct wl_cursor_image *image = app->wl.cursor->images[0];
    struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);
    if (!buffer) {
      perror("wl buffer");
      return ;
    }
    wl_pointer_set_cursor(pointer, serial, app->wl.cursor_surface, image->hotspot_x, image->hotspot_y);
    wl_surface_attach(app->wl.cursor_surface, buffer, 0, 0);
    wl_surface_damage(app->wl.cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(app->wl.cursor_surface);
  }
}

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface) {

  APP *app = (APP *)data;
  Monitor* m = find_monitor_by_surface(app, surface);
  if (!m) {
    return;
  }
  m->pointer_inside = 0;

}

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

static void frame_done(void *data, struct wl_callback *cb, uint32_t time){
  
  wl_callback_destroy(cb);
  Monitor *m = (Monitor *)data;
  m->pending_frame = 0;
  
  gl_draw(m->app, m);
  wl_surface_commit(m->surface);

  if (m->pointer_inside) {
    
    // gl_draw(m->app, m);

    m->frame_cb = wl_surface_frame(m->surface); 
    wl_callback_add_listener(m->frame_cb, &frame_listener, m);
    wl_surface_commit(m->surface);
    m->pending_frame = 1;
  }
}

void setupWayland(APP *app){
  app->wl.display = wl_display_connect(NULL);

  if (!app->wl.display) {
      fprintf(stderr, "[ERR][WAYLAND]: Failed to connect\n");
      exit(1);
  }
  app->wl.registry = wl_display_get_registry(app->wl.display);
  wl_registry_add_listener(app->wl.registry, &registry_listener, app);
  wl_display_roundtrip(app->wl.display);
  wl_display_roundtrip(app->wl.display);

  app->wl.pointer = wl_seat_get_pointer(app->wl.seat);
  wl_pointer_add_listener(app->wl.pointer, &pointer_listener, app);
}

void setupCursor(APP *app){

  if (app->wl.shm) {
    /* supports X_CURSOR only cause hyprcursor uses
     * different format for cursor and does not have API
     * to work with.
     */
    char* theme = getenv("XCURSOR_THEME");
    char* cursor_size = getenv("XCURSOR_SIZE");
    
    int size = cursor_size ? atoi(cursor_size) : 24;

    app->wl.cursor_theme = wl_cursor_theme_load(theme,size, app->wl.shm);
    if (app->wl.cursor_theme) {
      app->wl.cursor = wl_cursor_theme_get_cursor(app->wl.cursor_theme, "default");
    }
    if (!app->wl.cursor) {
      app->wl.cursor = wl_cursor_theme_get_cursor(app->wl.cursor_theme, "left_ptr");
    }
    if (app->wl.cursor) {
      app->wl.cursor_surface = wl_compositor_create_surface(app->wl.compositor);
    }
  }
}

void setupSurface(APP *app, Monitor *m){

  // getting surfaces and mouse pointer
  m->surface = wl_compositor_create_surface(app->wl.compositor);
  if (!m->surface) {
    fprintf(stderr, "[ERR][WL]: compositor failed to create surface for monitor : %d\n",m->global_name);
    return;
  }
  
  // getting layer shell surface
  m->layer_surface =
    zwlr_layer_shell_v1_get_layer_surface(app->wl.layer_shell, m->surface, m->output, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "Wallrift");

  // setting anchors for the surface
  zwlr_layer_surface_v1_set_anchor(m->layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

  zwlr_layer_surface_v1_set_size(m->layer_surface, m->width, m->height);

  // trying to bypass all the exclusive zones
  zwlr_layer_surface_v1_set_exclusive_zone(m->layer_surface, -1);

  // adding layer shell surface listener to layer shell surface
  zwlr_layer_surface_v1_add_listener(m->layer_surface, &layer_listener, m);

  // commiting the surface
  wl_surface_commit(m->surface);

  while (!m->configured) {
    wl_display_dispatch(app->wl.display);
  }

  m->frame_cb = wl_surface_frame(m->surface);
  wl_callback_add_listener(m->frame_cb, &frame_listener, m);
  wl_surface_commit(m->surface);
  m->pending_frame  = 1;
  printf("[INFO]: Monitor %d Display Width  = %d\n", m->global_name, m->width);
  printf("[INFO]: Monitor %d Display Height = %d\n", m->global_name, m->height);
}

void setupEGLGlobal(APP *app){
app->egl.egl_display = eglGetDisplay((EGLNativeDisplayType)app->wl.display);
  if (!eglInitialize(app->egl.egl_display, NULL, NULL)) {
      fprintf(stderr, "[ERR][EGL]: Init failed\n");
      exit(1);
  }
  EGLint attributes[] =
  {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  EGLConfig config;
  EGLint num_configs;

  if (!eglChooseConfig(app->egl.egl_display, attributes, &config, 1, &num_configs) || num_configs < 1) {
    fprintf(stderr,"\n[ERR][EGL]: Can't get egl config\n");
  }

  eglBindAPI(EGL_OPENGL_ES_API);

  app->egl.egl_config = config;
  app->egl.egl_context = eglCreateContext(app->egl.egl_display, app->egl.egl_config, EGL_NO_CONTEXT, ctxAttribs);

  if (app->egl.egl_context == EGL_NO_CONTEXT) {
      fprintf(stderr, "[ERR][EGL]: Failed to create egl context\n");
      exit(1);
  }
}

void setupEGL(APP *app, Monitor *m){
  m->egl_window = wl_egl_window_create(m->surface, m->width, m->height);

  if (!m->egl_window) {
    fprintf(stderr, "[ERR][EGL]: Failed to create egl window\n");
    exit(1);
  }
  
  EGLint attributes[] =
  {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  m->egl_surface = eglCreateWindowSurface(app->egl.egl_display, app->egl.egl_config, (EGLNativeWindowType)m->egl_window, NULL);

  if (m->egl_surface == EGL_NO_SURFACE) {
      fprintf(stderr, "[ERR][EGL]: Failed to create egl surface\n");
      exit(1);
  }

  // eglBindAPI(EGL_OPENGL_ES_API);
  if (!eglMakeCurrent(app->egl.egl_display, m->egl_surface, m->egl_surface, app->egl.egl_context)) {
      fprintf(stderr,"\n[ERR][EGL]: eglMakeCurrent failed\n");
    }
  // gl_draw(app, m);
}
