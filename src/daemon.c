/*
 author : github.com/saber-88
 */

#include <bits/time.h>
#include <fcntl.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <stddef.h>
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

#include "../include/wayland.h"
#include "../include/gl.h"
#include "../include/file.h"

#include "../include/app.h"
#include "monitor.h"

#define SOCK_PATH "/tmp/wallrift.sock"

double get_time(){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return  ts.tv_sec + ts.tv_nsec * 1e-9;
}

int setupDaemonSocket(){
  int daemon_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (daemon_sock == -1) {
    perror("Socket\n");
    return -1;
  }
  
  struct sockaddr_un d_addr = {0};
  d_addr.sun_family = AF_UNIX;
  strncpy(d_addr.sun_path, SOCK_PATH, sizeof(d_addr.sun_path) - 1);
  d_addr.sun_path[sizeof(d_addr.sun_path) - 1] = '\0';
  unlink(SOCK_PATH);

  if (bind(daemon_sock, (struct sockaddr*)&d_addr, sizeof(d_addr)) == -1) {
    fprintf(stderr,"[ERR][SOCK]: Failed to bind socket!");
    close(daemon_sock);
    return -1;
  }

  printf("[INFO]: Socket created\n");
  listen(daemon_sock, 5);
  printf("[INFO]: Listening...\n\n");
  int flags = fcntl(daemon_sock, F_GETFL, 0);
  fcntl(daemon_sock, F_SETFL, flags | O_NONBLOCK);
  return daemon_sock;
}

void handleClient(int daemon_sock, char *wallpath, size_t wallSize, APP *app){
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
             app->gl.speed = strtof(tok, NULL);
             if (app->gl.speed <= 0.00) app->gl.speed = 0.00f;
             if (app->gl.speed >= 1.00) app->gl.speed = 1.00f;
           }
        }
        tok = strtok(NULL, " ");
      }

      if (path && strcmp(path, wallpath) != 0) {
        snprintf(wallpath, wallSize, "%s",path);
        GLuint nexTex = loadImageIntoGPU(wallpath, &app->active_monitor->img_w, &app->active_monitor->img_h, app->active_monitor->textureId);
        cache_wallpaper(wallpath);
        if (app->active_monitor->textureId != nexTex) {
          if (app->active_monitor->textureId != 0) {
            glDeleteTextures(1, &app->active_monitor->textureId);
          }
          app->active_monitor->textureId = nexTex;
        }
        gl_draw(app, app->active_monitor);
      }
    }
  }
}

int main() {
  
  APP *app = calloc(1, sizeof(APP));

  WLGlobal wl;
  GL gl;
  /* setting initial speed to 0.05 for smooth parallax
   * i need to implement config file support soon so 
   * that i can read from config file instead of 
   * hardcoding values.
   */
  gl.speed = 0.05f;

  app->wl = wl;
  app->gl = gl;

  setupWayland(app);
  setupCursor(app);
  setupEGLGlobal(app);
  
  // setting up surface and egl for each monitor
  for (int i = 0; i < app->monitor_count; i++) {
    setupSurface(app, &app->monitors[i]);
    setupEGL(app, &app->monitors[i]);
    printf("setup done for monitor %d, id %d\n",i,app->monitors[i].global_name);
  }

  /* making context current before opengGL setup
   * because nvidia likes it this way
   */

  eglMakeCurrent(app->egl.egl_display, app->monitors[0].surface, app->monitors[0].surface,app->egl.egl_context);
  
  // setting up OpenGL
  if (setupOpenGL(app)) {
    fprintf(stderr, "[ERR][GL]: Failed to setup OpenGL\n");
    return 1;
  }
  
  app->gl.texLoc = glGetUniformLocation(app->gl.prog, "tex");

  // loading cached wallpaper
  char wallpath[5000];
  const char* cached = get_cached_wallpaper();
  if (cached) {
    snprintf(wallpath, sizeof(wallpath), "%s", cached);
  }
  else {
    wallpath[0] = '\0';
  }
  
  // initially loading cached wallpaper for every monitor
  for (int i = 0; i < app->monitor_count; i++) {
    Monitor *m = &app->monitors[i];
    eglMakeCurrent(app->egl.egl_context, m->egl_surface, m->egl_surface, app->egl.egl_context);
    m->textureId = loadImageIntoGPU(wallpath, &m->img_w, &m->img_h, m->textureId);
  }
  
  // initially drawing cached wallpaper for every monitor
  for (int i = 0; i < app->monitor_count; i++) {
    gl_draw(app, &app->monitors[i]);
  }

  int daemon_sock = setupDaemonSocket();
  if (daemon_sock == -1) {
    fprintf(stderr, "[ERR][SOCK]: %s\n","Failed to setup daemon socket");
    return 1;
  }

  // getting stuff ready for polling
  app->wl.wayland_fd = wl_display_get_fd(app->wl.display);
  struct pollfd fds[2];
  fds[0].fd = app->wl.wayland_fd;
  fds[0].events = POLLIN;

  fds[1].fd = daemon_sock;
  fds[1].events = POLLIN;
  int did_read = 0;
  

  // render loop
  while (1) {
    did_read = 0;
    wl_display_dispatch_pending(app->wl.display);

    if (wl_display_prepare_read(app->wl.display) != 0) {
      wl_display_dispatch_pending(app->wl.display);
      continue;
    }
    wl_display_flush(app->wl.display);
    int timeout = app->active_monitor->pointer_inside ? 0 : -1;
    int ret = poll(fds, 2, timeout);

    if (ret > 0) {
      // wayland event happend
      if (fds[0].revents & POLLIN) {
        wl_display_read_events(app->wl.display);
        did_read = 1;
      }
      
      // socket event happend
      if (fds[1].revents & POLLIN) {
        handleClient(daemon_sock, wallpath, sizeof(wallpath), app);
      }
    }
    if (!did_read) {
      wl_display_cancel_read(app->wl.display);
    }
  }
  close(daemon_sock);
  unlink(SOCK_PATH);
  return 0;
}
