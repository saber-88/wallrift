#include "wayland.h"
#include "ext-idle-notify-v1-client-protocol.h"
#include "file.h"
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
#include <wayland-util.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>
#include "log.h"

static void frame_done(void *data, struct wl_callback *cb, uint32_t time);

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
  (void)version;
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
  else if (!strcmp(interface, "ext_idle_notifier_v1")) {
    app->wl.notifier = wl_registry_bind(registry, id, &ext_idle_notifier_v1_interface, 1);

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

    if (app->egl.egl_display != EGL_NO_DISPLAY && app->egl.egl_context != EGL_NO_CONTEXT) {
      setupSurface(app, m);
      setupEGL(app, m);
      if (app->monitors[0].textureId != 0) {
        char* wall = (char *)get_cached_wallpaper();
        if (wall) {
          eglMakeCurrent(app->egl.egl_display, m->egl_surface, m->egl_surface, app->egl.egl_context);
          m->textureId = loadImageIntoGPU(wall, &m->img_w, &m->img_h, 0);
        }
        gl_draw(app, m);
        LOG_INFO("WL", "Hotplugged monitor id: %d set up done",m->global_name);
      }
    }
  }
}

static void registry_remove(void *data, struct wl_registry *registry,
                            uint32_t id) {
  (void)registry;
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
          m->pending_frame = 0;
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


static void on_idle(void *data, struct ext_idle_notification_v1 *notif){
  (void)notif;
  APP *app = (APP *)data;
  app->wl.cursor_moved = 0;
  printf("Stale cursor\n");

};

static void on_resume(void *data, struct ext_idle_notification_v1 *notif){
  (void)notif;
  APP *app = (APP *)data;
  Monitor *m = app->active_monitor;
  app->wl.cursor_moved = 1;
  printf("resumed\n");

  if (!m->pending_frame) {
      m->frame_cb = wl_surface_frame(m->surface);
      wl_callback_add_listener(m->frame_cb, &frame_listener, m);
      m->pending_frame = 1;
      wl_surface_commit(m->surface);
  }
};

static const struct ext_idle_notification_v1_listener idle_notif_listener = {
  .idled = on_idle,
  .resumed = on_resume,
};

static void layer_closed(void *data, struct zwlr_layer_surface_v1 *surface){
  (void)data;
  (void)surface;
}

// layer-shell surface listener 
static const struct zwlr_layer_surface_v1_listener layer_listener= {
  layer_configure,
  layer_closed
};


// pointer stuff
static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy){
  (void)pointer;
  (void)time;
  (void)sy;

  APP *app = (APP *)data;
  if (!app->active_monitor) {
    LOG_WARN("WL","No active monitor found");
    return;
  }

  double cx = wl_fixed_to_double(sx);
  double cy = wl_fixed_to_double(sy);

  app->wl.last_cursor_x = cx;
  app->wl.last_cursor_y = cy;
  app->active_monitor->target_cursor = cx;

  float normalized = app->active_monitor->target_cursor / app->active_monitor->width;
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  app->active_monitor->target_cursor = normalized;
}
static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t sx, wl_fixed_t sy) {
  (void)sy;
  APP *app = (APP *)data;
  Monitor *m = find_monitor_by_surface(app, surface);
  if (!m) {

    LOG_ERR("WL","No monitor found by surface");
    return;
  }
  app->active_monitor = m;
  app->active_monitor->pointer_inside = 1;

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

  (void)pointer;
  (void)serial;
  APP *app = (APP *)data;
  Monitor* m = find_monitor_by_surface(app, surface);
  if (!m) {
    return;
  }
  m->pointer_inside = 0;

}

static void pointer_button(void *data, struct wl_pointer *pointer,
                           uint32_t serial, uint32_t time,
                           uint32_t button, uint32_t state) {
    (void)data;
    (void)pointer;
    (void)serial;
    (void)time;
    (void)button;
    (void)state;
}

static void pointer_axis(void *data, struct wl_pointer *pointer,
                         uint32_t time, uint32_t axis, wl_fixed_t value) {
    (void)data;
    (void)pointer;
    (void)time;
    (void)axis;
    (void)value;
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_enter,
    pointer_leave,
    pointer_motion,
    pointer_button,
    pointer_axis,
    .frame = NULL
};

static void frame_done(void *data, struct wl_callback *cb, uint32_t time){
  
  (void)time;

  wl_callback_destroy(cb);
  Monitor *m = (Monitor *)data;
  m->frame_cb = NULL;
  m->pending_frame = 0;

  if (!m->surface || !m->egl_surface) {
    return;
  }
  if (m->app->wl.cursor_moved == 0) {
    printf("skip render\n");
    return;
  }

  gl_draw(m->app, m);
  wl_surface_commit(m->surface);

  if (m->pointer_inside) {
    m->frame_cb = wl_surface_frame(m->surface); 
    wl_callback_add_listener(m->frame_cb, &frame_listener, m);
    wl_surface_commit(m->surface);
    m->pending_frame = 1;
  }
}

void setupWayland(APP *app){
  app->wl.display = wl_display_connect(NULL);

  if (!app->wl.display) {
      LOG_ERR("WL","Failed to connect to display\n");
      exit(1);
  }
  app->wl.registry = wl_display_get_registry(app->wl.display);
  wl_registry_add_listener(app->wl.registry, &registry_listener, app);

  wl_display_roundtrip(app->wl.display);
  wl_display_roundtrip(app->wl.display);

  app->wl.pointer = wl_seat_get_pointer(app->wl.seat);
  wl_pointer_add_listener(app->wl.pointer, &pointer_listener, app);

  app->wl.notification = ext_idle_notifier_v1_get_idle_notification(app->wl.notifier, 1000, app->wl.seat);
  ext_idle_notification_v1_add_listener(app->wl.notification, &idle_notif_listener, app);
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
    LOG_ERR("WL", "Compositor failed to create surface for monitor : %d\n",m->global_name);
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
  LOG_INFO("WL", "Monitor id: %d, width: %d, height: %d", m->global_name, m->width, m->height);
}

void setupEGLGlobal(APP *app){
app->egl.egl_display = eglGetDisplay((EGLNativeDisplayType)app->wl.display);
  if (!eglInitialize(app->egl.egl_display, NULL, NULL)) {
      LOG_ERR("EGL", "Init failed");
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
    LOG_ERR("EGL", "Cannot get egl config");
    return;
  }

  eglBindAPI(EGL_OPENGL_ES_API);

  app->egl.egl_config = config;
  app->egl.egl_context = eglCreateContext(app->egl.egl_display, app->egl.egl_config, EGL_NO_CONTEXT, ctxAttribs);

  if (app->egl.egl_context == EGL_NO_CONTEXT) {
      LOG_ERR("EGL", "Failed to create egl context");
      exit(1);
  }
}

void setupEGL(APP *app, Monitor *m){
  m->egl_window = wl_egl_window_create(m->surface, m->width, m->height);

  if (!m->egl_window) {
    LOG_ERR("EGL", "Failed to create egl window");
    exit(1);
  }

  m->egl_surface = eglCreateWindowSurface(app->egl.egl_display, app->egl.egl_config, (EGLNativeWindowType)m->egl_window, NULL);

  if (m->egl_surface == EGL_NO_SURFACE) {
      LOG_ERR("EGL", "Failed to create egl surface");
      exit(1);
  }

  if (!eglMakeCurrent(app->egl.egl_display, m->egl_surface, m->egl_surface, app->egl.egl_context)) {
      LOG_ERR("EGL", "eglMakeCurrent failed");
    }
}
