#include "../include/wayland.h"

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
  else if (!strcmp(interface, "wl_shm")) {
    wl->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
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
  WL *wl = (WL *)data;
  wl->run = 0;
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
  wl->target_cursor = wl_fixed_to_double(sx);

  float normalized = wl->target_cursor / wl->width;
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  wl->target_cursor = normalized;
}
static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t sx, wl_fixed_t sy) {

  WL *wl = (WL *)data;
  wl->target_cursor = wl_fixed_to_double(sx);

  float normalized = wl->target_cursor / wl->width;
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  wl->target_cursor = normalized;

  if (wl->cursor && wl->cursor_surface) {
    struct wl_cursor_image *image = wl->cursor->images[0];
    wl_pointer_set_cursor(pointer, serial, wl->cursor_surface, image->hotspot_x, image->hotspot_y);
    wl_surface_attach(wl->cursor_surface, wl_cursor_image_get_buffer(image), 0, 0);
    wl_surface_damage(wl->cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(wl->cursor_surface);
  }
  wl->run = 1;
}

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface) {
  WL *wl = (WL *)data;

  wl->run = 0;
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

//callback stuff

static void frame_done(void *data, struct wl_callback *cb, uint32_t time){
  wl_callback_destroy(cb);

  
}

static const struct wl_callback_listener frame_listener = {
  .done = frame_done 

};

void setupWayland(WL *wl){
  wl->run = 1;
  wl->cursor_x = 0.05f; // initialize it at centre 
  wl->display = wl_display_connect(NULL);
  if (!wl->display) {
      fprintf(stderr, "[ERR][WAYLAND]: Failed to connect\n");
      exit(1);
  }
  wl->registry = wl_display_get_registry(wl->display);
  wl_registry_add_listener(wl->registry, &registry_listener, wl);
  wl_display_roundtrip(wl->display);
}

void setupCursor(WL *wl){

  if (wl->shm) {
    /* supports X_CURSOR only cause hyprcursor uses
     * different format for cursor and does not have API
     * to work with.
     */
    char* theme = getenv("XCURSOR_THEME");
    char* cursor_size = getenv("XCURSOR_SIZE");
    
    int size = cursor_size ? atoi(cursor_size) : 24;

    wl->cursor_theme = wl_cursor_theme_load(theme,size, wl->shm);
    if (wl->cursor_theme) {
      wl->cursor = wl_cursor_theme_get_cursor(wl->cursor_theme, "default");
    }
    if (!wl->cursor) {
      wl->cursor = wl_cursor_theme_get_cursor(wl->cursor_theme, "left_ptr");
    }
    if (wl->cursor) {
      wl->cursor_surface = wl_compositor_create_surface(wl->compositor);
    }
  }
}

void setupSurface(WL *wl){

  // getting surfaces and mouse pointer
  wl->surface = wl_compositor_create_surface(wl->compositor);
  wl->pointer = wl_seat_get_pointer(wl->seat);
  wl_pointer_add_listener(wl->pointer, &pointer_listener, wl);

  struct wl_callback *cb = wl_surface_frame(wl->surface);
  wl_callback_add_listener(cb, &frame_listener, wl);


  // getting layer shell surface
  wl->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      wl->layer_shell, wl->surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
      "wallpaper");

  // setting anchors for the surface
  zwlr_layer_surface_v1_set_anchor(wl->layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

  zwlr_layer_surface_v1_set_size(wl->layer_surface, 0, 0);
  // trying to bypass all the exclusive zones
  zwlr_layer_surface_v1_set_exclusive_zone(wl->layer_surface, -1);

  // adding layer shell surface listener to layer shell surface
  zwlr_layer_surface_v1_add_listener(wl->layer_surface, &layer_listener, wl);

  // commiting the surface
  wl_surface_commit(wl->surface);

  while (!wl->configured) {
    wl_display_dispatch(wl->display);
  }

  printf("\n\n%s %d\n", "[INFO]: Display Width = ", wl->width);
  printf("%s %d\n\n", "[INFO]: Display Height = ", wl->height);
}

void setupEGL(WL *wl){
  wl->egl_window = wl_egl_window_create(wl->surface, wl->width, wl->height);
  if (!wl->egl_window) {
      fprintf(stderr, "[ERR][EGL]: Failed to create egl window\n");
      exit(1);
  }
  wl->egl_display = eglGetDisplay((EGLNativeDisplayType)wl->display);
  if (!eglInitialize(wl->egl_display, NULL, NULL)) {
      fprintf(stderr, "[ERR][EGL]: Init failed\n");
      exit(1);
  }

  EGLint attributes[] =
  {
    EGL_SURFACE_TYPE,
    EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  EGLConfig config;
  EGLint num_configs;
  if (!eglChooseConfig(wl->egl_display, attributes, &config, 1, &num_configs) || num_configs < 1) {
    fprintf(stderr,"\n[ERR][EGL]: Can't get egl config\n");
  }

  /* this will allocate gpu framebuffers,
   * egl_surface is where the gpu will render
  */
  wl->egl_surface = 
    eglCreateWindowSurface(wl->egl_display, config, (EGLNativeWindowType)wl->egl_window, NULL);

  if (wl->egl_surface == EGL_NO_SURFACE) {
      fprintf(stderr, "[ERR][EGL]: Failed to create surface\n");
      exit(1);
  }

  eglBindAPI(EGL_OPENGL_ES_API);

  EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  wl->egl_context = eglCreateContext(wl->egl_display, config, EGL_NO_CONTEXT, ctxAttribs);
  if (wl->egl_context == EGL_NO_CONTEXT) {
      fprintf(stderr, "[ERR][EGL]: Failed to create context\n");
      exit(1);
  }
  if (!eglMakeCurrent(wl->egl_display, wl->egl_surface, wl->egl_surface, wl->egl_context)) {
    fprintf(stderr,"\n[ERR][EGL]: eglMakeCurrent failed\n");
  }
}
