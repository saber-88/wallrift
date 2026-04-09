/*
 author : github.com/saber-88
 */

#include "../include/gl.h"
#include "../include/wayland.h"

#include <fcntl.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>

#define SOCK_PATH "/tmp/wallrift.sock"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

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
    printf("\nCannot load image\n");
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
  printf("\nImage loaded in GPU\n");
  printf("\nImg width = %d\nImg height = %d\n", *imageWidth, *imageHeight);
  return texID;
}

char *readFile(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("Failed to open file\n");
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  char *data = malloc(size + 1);
  fread(data, 1, size, f);
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
    printf("[ERR][shader]:%s\n", infoLog);
  }
  return shader;
}

GLuint createProgram(const char *vFilePath, const char *fFilePath) {
  const char *vsrc = readFile(vFilePath);
  const char *fsrc = readFile(fFilePath);
  GLuint vs = createShader(GL_VERTEX_SHADER, vsrc);
  GLuint fs = createShader(GL_FRAGMENT_SHADER, fsrc);
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
    printf("[ERR][linking]:%s\n", infoLog);
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}

// wallpaper coordinates
float vertices[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 1.0f,
    1.0f,  1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 0.0f,
};

unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

float speed = 0.03f;

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
    wl_display_dispatch(wl->display);

}
int main() {

  char *wallpath = NULL;
  char *pendingPath = NULL;
  WL wl = {0};
  GL gl = {0};
  wl.display = wl_display_connect(NULL);
  wl.registry = wl_display_get_registry(wl.display);
  wl_registry_add_listener(wl.registry, &registry_listener, &wl);
  wl_display_roundtrip(wl.display);

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

  // adding layer shell surface listener to layer shell surface
  zwlr_layer_surface_v1_add_listener(wl.layer_surface, &layer_listener, &wl);

  // commiting the surface
  wl_surface_commit(wl.surface);

  while (!wl.configured) {
    wl_display_dispatch(wl.display);
  }

  printf("\n%s %d\n", "Display Width = ", wl.width);
  printf("%s %d\n", "Display Height = ", wl.height);

  // egl stuff

  wl.egl_window = wl_egl_window_create(
      wl.surface, wl.width, wl.height); // encapsulate wl_surface in egl_surface
  wl.egl_display =
      eglGetDisplay((EGLNativeDisplayType)wl.display); // connect egl to wayland
  eglInitialize(wl.egl_display, NULL, NULL);

  EGLint attributes[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,
                         EGL_OPENGL_ES2_BIT, EGL_NONE};

  EGLConfig config;
  EGLint num_configs;
  if (!eglChooseConfig(wl.egl_display, attributes, &config, 1, &num_configs)) {
    printf("\nERR: Cant get egl config\n");
  }

  // this will allocate gpu framebuffers, egl_surface is where the gpu will
  // render
  wl.egl_surface = eglCreateWindowSurface(
      wl.egl_display, config, (EGLNativeWindowType)wl.egl_window, NULL);

  eglBindAPI(EGL_OPENGL_ES_API);

  EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  wl.egl_context =
      eglCreateContext(wl.egl_display, config, EGL_NO_CONTEXT, ctxAttribs);

  if (!eglMakeCurrent(wl.egl_display, wl.egl_surface, wl.egl_surface,
                      wl.egl_context)) {
    printf("\neglMakeCurrent failed\n");
  }




  // GLSL stuff
  
  GLuint textureId = 0;
  // GLuint textureId = loadImageIntoGPU(wallpath, &img_w , &img_h);
  gl.prog =
      createProgram("/usr/share/wallrift/shaders/wallpaper.vert",
                    "/usr/share/wallrift/shaders/wallpaper.frag");

  gl.cursorLoc = glGetUniformLocation(gl.prog, "u_cursor");
  gl.imgWLoc = glGetUniformLocation(gl.prog, "u_img_width");
  gl.imgHLoc = glGetUniformLocation(gl.prog, "u_img_height");
  gl.viewWLoc = glGetUniformLocation(gl.prog, "u_view_width");
  gl.viewHLoc  = glGetUniformLocation(gl.prog, "u_view_height");
  // unix socket
  int daemon_sock = socket(AF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un d_addr = {0};
  d_addr.sun_family = AF_UNIX;
  strncpy(d_addr.sun_path, SOCK_PATH, sizeof(d_addr.sun_path) - 1);
  unlink(SOCK_PATH);

  if (bind(daemon_sock, (struct sockaddr*)&d_addr, sizeof(d_addr)) == -1) {
    perror("Failed to bind socket!");
    close(daemon_sock);
    return 1;
  }

  printf("Socket created\n");
  listen(daemon_sock, 5);
  printf("Listening...\n");
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


  while (1) {

    int client = accept(daemon_sock, NULL, NULL);
    if (client != -1) {
      char buff[1024];
      int n = read(client, buff, sizeof(buff)-1);
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
        if (path) {
          pendingPath = strdup(path);
          textureId = loadImageIntoGPU(pendingPath, &img_w, &img_h, textureId);
          free(pendingPath);
          pendingPath = NULL;
        }
      }
    }
    gl_draw(&wl, &gl, texloc, textureId);
  }

  close(daemon_sock);
  unlink(SOCK_PATH);
  return 0;
}
