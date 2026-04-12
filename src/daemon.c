/*
 author : github.com/saber-88
 */

#include "../include/gl.h"
#include "../include/wayland.h"

#include <bits/time.h>
#include <fcntl.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <time.h>

#define SOCK_PATH "/tmp/wallrift.sock"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"


double get_time(){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return  ts.tv_sec + ts.tv_nsec * 1e-9;
}

const char* get_cache_file(){
  const char* xdg = getenv("XDG_CACHE_HOME");
  static char path[1024];
  if (xdg && *xdg) {
    snprintf(path, sizeof(path), "%s/wallrift/current",xdg);
  }
  else {
    snprintf(path, sizeof(path), "%s/.cache/wallrift/current",getenv("HOME"));
  }
  return path;
}

void cache_wallpaper(const char* wallpaper){
  const char* file = get_cache_file();
  char dir[1024];
  snprintf(dir, sizeof(dir), "%s",file);
  char *slash = strrchr(dir, '/');
  if (slash) {
    *slash = '\0';
    mkdir(dir, 0755);
  }

  FILE *f = fopen(file, "w");
  if (f) {
    fprintf(f, "%s\n",wallpaper);
    fclose(f);
  }
}

const char* get_cached_wallpaper() {
    static char wall[5000];
    const char *cache_file = get_cache_file();
    FILE *f = fopen(cache_file, "r");
    if (!f) {
        return NULL;
    }
    if (!fgets(wall, sizeof(wall), f)) {
        fclose(f);
        return NULL;
    }
    // remove newline
    wall[strcspn(wall, "\n")] = '\0';
    fclose(f);
    return wall;
}

GLuint loadImageIntoGPU(char *imgPath, int *imageWidth, int* imageHeight, GLuint texID) {

  char expanded[1024];
  if (imgPath[0] == '~') {
    snprintf(expanded, sizeof(expanded), "%s%s", getenv("HOME"), &imgPath[1]);
    printf("%s", expanded);
  }
  else {
    snprintf(expanded, sizeof(expanded), "%s", imgPath);
  }
  int channels;
  unsigned char *pixels = stbi_load(expanded, imageWidth, imageHeight, &channels, 4);
  if (!pixels) {
    printf("\n[ERR][STB]: Cannot load image\n");
    return texID;
  }

  if (texID == 0) {
    glGenTextures(1, &texID);
  }
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *imageWidth, *imageHeight, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(pixels);
  printf("[INFO]: Image loaded in GPU\n");
  printf("[INFO]: Img width = %d\n[INFO]: Img height = %d\n\n", *imageWidth, *imageHeight);
  return texID;
}

char *readFile(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("[ERR][FILE]: Failed to open file\n");
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  char *data = malloc(size + 1);
  size_t read_bytes = fread(data, 1, size, f);
  if (read_bytes != size) {
    printf("[ERR][FILE]: failed to read file completely\n");
    free(data);
    fclose(f);
    return NULL;
  }
  data[size] = '\0';

  fclose(f);
  return data;
}

GLuint createShader(GLenum type, const char *shaderSrc) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &shaderSrc, NULL);
  glCompileShader(shader);
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    printf("[ERR][SHADER]: %s\n", infoLog);
    return -1;
  }
  return shader;
}

GLuint createProgram(const char *vFilePath, const char *fFilePath) {
  const char *vsrc = readFile(vFilePath);
  const char *fsrc = readFile(fFilePath);
  if (!vsrc || !fsrc) {
    printf("[ERR][FILE]: %s\n", "Failed to open shader file.");
    return -1;
  }
  GLuint vs = createShader(GL_VERTEX_SHADER, vsrc);
  GLuint fs = createShader(GL_FRAGMENT_SHADER, fsrc);

  if (vs == -1 || fs == -1) {
    printf("[ERR][SHADER]: %s\n", "Failed to create shader.");
    return -1;
  }
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);

  free((void *)vsrc);
  free((void *)fsrc);

  int success;
  char infoLog[512];
  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(prog, 512, NULL, infoLog);
    printf("[ERR][LINKING] :%s\n", infoLog);
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}

/* wallpaper coordinates x , y , z , w
 * x , y are display cooridantes with (0,0) as centre.
 * z , w are image coordinates with (0,0)  as bottom left.
*/
float vertices[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 1.0f,
    1.0f,  1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 0.0f,
};

// these are the indices to draw vertices in the given order
unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

// speed factor is inversely proportional to smoothness of parallax / panning movement
float speed = 0.03f;

// wallpaper width and height
int img_w , img_h;

void gl_draw(WL *wl, GL *gl, int texloc, GLuint textureId){
    
    glViewport(0, 0, wl->width, wl->height);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(gl->prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(texloc, 0);

    // update mouse constatnly for parallax
    wl->cursor_x += (wl->target_cursor - wl->cursor_x) * speed;
    glUniform1f(gl->cursorLoc, wl->cursor_x);
    glUniform1f(gl->imgWLoc, (float)img_w);
    glUniform1f(gl->viewWLoc, (float)wl->width);
    glUniform1f(gl->imgHLoc, (float)img_h);
    glUniform1f(gl->viewHLoc, (float)wl->height);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    eglSwapBuffers(wl->egl_display, wl->egl_surface);

}
int main() {

  char wallpath[5000];
  const char* cached = get_cached_wallpaper();
  if (cached) {
    snprintf(wallpath, sizeof(wallpath), "%s", cached);
  }
  else {
    wallpath[0] = '\0';
  }


  WL wl = {0};
  GL gl = {0};
  wl.run = 1;
  wl.cursor_x = 0.05f; // initialize it at centre 
  wl.display = wl_display_connect(NULL);
  wl.registry = wl_display_get_registry(wl.display);
  wl_registry_add_listener(wl.registry, &registry_listener, &wl);
  wl_display_roundtrip(wl.display);

  // cursor stuff
  if (wl.shm) {
    
    /* supports X_CURSOR only cause hyprcursor uses
     * different format for cursor and does not have API
     * to work with.
     */
    char* theme = getenv("XCURSOR_THEME");
    char* cursor_size = getenv("XCURSOR_SIZE");
    
    int size = cursor_size ? atoi(cursor_size) : 24;

    wl.cursor_theme = wl_cursor_theme_load(theme,size, wl.shm);
    if (wl.cursor_theme) {
      wl.cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "default");
    }
    if (!wl.cursor) {
      wl.cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "left_ptr");
    }
    if (wl.cursor) {
      wl.cursor_surface = wl_compositor_create_surface(wl.compositor);
    }
  }

  // getting surfaces and mouse pointer
  wl.surface = wl_compositor_create_surface(wl.compositor);
  wl.pointer = wl_seat_get_pointer(wl.seat);
  wl_pointer_add_listener(wl.pointer, &pointer_listener, &wl);

  // getting layer shell surface
  wl.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      wl.layer_shell, wl.surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
      "wallpaper");

  // setting anchors for the surface
  zwlr_layer_surface_v1_set_anchor(wl.layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

  zwlr_layer_surface_v1_set_size(wl.layer_surface, 0, 0);
  // trying to bypass all the exclusive zones
  zwlr_layer_surface_v1_set_exclusive_zone(wl.layer_surface, -1);

  // adding layer shell surface listener to layer shell surface
  zwlr_layer_surface_v1_add_listener(wl.layer_surface, &layer_listener, &wl);

  // commiting the surface
  wl_surface_commit(wl.surface);

  while (!wl.configured) {
    wl_display_dispatch(wl.display);
  }

  printf("\n\n%s %d\n", "[INFO]: Display Width = ", wl.width);
  printf("%s %d\n\n", "[INFO]: Display Height = ", wl.height);

  // egl stuff
  wl.egl_window = wl_egl_window_create(wl.surface, wl.width, wl.height); // encapsulate wl_surface in egl_surface
  wl.egl_display = eglGetDisplay((EGLNativeDisplayType)wl.display); // connect egl to wayland
  eglInitialize(wl.egl_display, NULL, NULL);

  EGLint attributes[] = {
                          EGL_SURFACE_TYPE,
                          EGL_WINDOW_BIT,
                          EGL_RENDERABLE_TYPE,
                          EGL_OPENGL_ES2_BIT,
                          EGL_NONE
                        };

  EGLConfig config;
  EGLint num_configs;
  if (!eglChooseConfig(wl.egl_display, attributes, &config, 1, &num_configs)) {
    printf("\n[ERR][EGL]: Cant get egl config\n");
  }

  /* this will allocate gpu framebuffers,
   * egl_surface is where the gpu will render
  */
  wl.egl_surface = eglCreateWindowSurface(
      wl.egl_display, config, (EGLNativeWindowType)wl.egl_window, NULL);

  eglBindAPI(EGL_OPENGL_ES_API);

  EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  wl.egl_context =
      eglCreateContext(wl.egl_display, config, EGL_NO_CONTEXT, ctxAttribs);

  if (!eglMakeCurrent(wl.egl_display, wl.egl_surface, wl.egl_surface, wl.egl_context)) {
    printf("\n[ERR][EGL]: eglMakeCurrent failed\n");
  }
  
  // GLSL stuff
  
  GLuint textureId = 0;

  // load the cached wallpaper
  if (wallpath[0] != '\0') {
     textureId = loadImageIntoGPU(wallpath, &img_w , &img_h, textureId);
     cache_wallpaper(wallpath);
  }

  if ((gl.prog =
      createProgram("/usr/share/wallrift/shaders/wallpaper.vert",
                    "/usr/share/wallrift/shaders/wallpaper.frag")) == -1) {
    
    printf("\n[ERR][GL]: Failed to create program.\n");
    return 1;
  }
  

  gl.cursorLoc = glGetUniformLocation(gl.prog, "u_cursor");
  gl.imgWLoc = glGetUniformLocation(gl.prog, "u_img_width");
  gl.imgHLoc = glGetUniformLocation(gl.prog, "u_img_height");
  gl.viewWLoc = glGetUniformLocation(gl.prog, "u_view_width");
  gl.viewHLoc = glGetUniformLocation(gl.prog, "u_view_height");

  // unix socket
  int daemon_sock = socket(AF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un d_addr = {0};
  d_addr.sun_family = AF_UNIX;
  strncpy(d_addr.sun_path, SOCK_PATH, sizeof(d_addr.sun_path) - 1);
  unlink(SOCK_PATH);

  if (bind(daemon_sock, (struct sockaddr*)&d_addr, sizeof(d_addr)) == -1) {
    perror("[ERR][SOCK]: Failed to bind socket!");
    close(daemon_sock);
    return 1;
  }

  printf("[INFO]: Socket created\n");
  listen(daemon_sock, 5);
  printf("[INFO]: Listening...\n\n");
  int flags = fcntl(daemon_sock, F_GETFL, 0);
  fcntl(daemon_sock, F_SETFL, flags | O_NONBLOCK);

  glUseProgram(gl.prog);
  glGenBuffers(1, &gl.vbo);
  glGenBuffers(1, &gl.ebo);

  glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  int texloc = glGetUniformLocation(gl.prog, "tex");
  int resLoc = glGetUniformLocation(gl.prog, "resolution");
  glUniform2f(resLoc, wl.width, wl.height);
  int imgLoc = glGetUniformLocation(gl.prog, "imgSize");
  glUniform2f(imgLoc, img_w, img_h);

  // getting stuff ready for polling
  wl.wayland_fd = wl_display_get_fd(wl.display);

  struct pollfd fds[2];

  fds[0].fd = wl.wayland_fd;
  fds[0].events = POLLIN;

  fds[1].fd = daemon_sock;
  fds[1].events = POLLIN;
  int did_read = 0;
  
  // render loop
  while (1) {

    did_read = 0;
    wl_display_dispatch_pending(wl.display);

    if (wl_display_prepare_read(wl.display) != 0) {
      wl_display_dispatch_pending(wl.display);
      continue;
    }
    wl_display_flush(wl.display);
    int ret = poll(fds, 2, 8);

    if (ret > 0) {
      // wayland event happend
      if (fds[0].revents & POLLIN) {
        wl_display_read_events(wl.display);
        did_read = 1;
      }
      
      // socket event happend
      if (fds[1].revents & POLLIN) {
        int client = accept(daemon_sock, NULL, NULL);
        if (client != -1) {
          char buff[1024];
          int n = read(client, buff, sizeof(buff)-1);
          close(client);
          if (n > 0) {
            buff[n] = '\0';
            char *path = NULL;
            char *tok = strtok(buff, " ");

            while (tok) {
              if (strcmp(tok, "img") == 0) {
                tok = strtok(NULL, " ");
                if (tok) {
                  path = tok;
                }
              }
              else if (strcmp(tok, "speed") == 0) {
                 tok = strtok(NULL, " ");
                 if (tok) {
                   speed = strtof(tok, NULL);
                   if (speed <= 0.00) speed = 0.00f;
                   if (speed >= 1.00) speed = 1.00f;
                 }
              }
              tok = strtok(NULL, " ");
            }

            if (path && strcmp(path, wallpath) != 0) {
              snprintf(wallpath, sizeof(wallpath), "%s",path);
              if (textureId != 0) {
                glDeleteTextures(1, &textureId);
              }
              textureId = loadImageIntoGPU(wallpath, &img_w, &img_h, textureId);
              cache_wallpaper(wallpath);
              gl_draw(&wl, &gl, texloc, textureId);
            }
          }
        }
      }
    }
    if (!did_read) {
      wl_display_cancel_read(wl.display);
    }
    // render only if the focus is on wallpaper for cpu / gpu efficiency
    if (wl.run) {
      gl_draw(&wl, &gl, texloc, textureId);
    }
  }
  close(daemon_sock);
  unlink(SOCK_PATH);
  return 0;
}
