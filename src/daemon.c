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

#include "../include/gl.h"
#include "../include/wayland.h"
#include "../include/file.h"


#define SOCK_PATH "/tmp/wallrift.sock"

double get_time(){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return  ts.tv_sec + ts.tv_nsec * 1e-9;
}
// speed factor is inversely proportional to smoothness of parallax / panning movement

// wallpaper width and height


int setupDaemonSocket(int daemon_sock){
  struct sockaddr_un d_addr = {0};
  d_addr.sun_family = AF_UNIX;
  strncpy(d_addr.sun_path, SOCK_PATH, sizeof(d_addr.sun_path) - 1);
  d_addr.sun_path[sizeof(d_addr.sun_path) - 1] = '\0';
  unlink(SOCK_PATH);

  if (bind(daemon_sock, (struct sockaddr*)&d_addr, sizeof(d_addr)) == -1) {
    fprintf(stderr,"[ERR][SOCK]: Failed to bind socket!");
    close(daemon_sock);
    return 1;
  }

  printf("[INFO]: Socket created\n");
  listen(daemon_sock, 5);
  printf("[INFO]: Listening...\n\n");
  int flags = fcntl(daemon_sock, F_GETFL, 0);
  fcntl(daemon_sock, F_SETFL, flags | O_NONBLOCK);
  return 0;
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
  gl.speed = 0.05f;
  gl.img_w = 0;
  gl.img_h = 0;
  setupWayland(&wl);
  setupCursor(&wl);
  setupSurface(&wl);
  setupEGL(&wl);
  
  // openGL stuff
  gl.textureId = 0;
  gl.texLoc = glGetUniformLocation(gl.prog, "tex");

  // load the cached wallpaper
  if (wallpath[0] != '\0') {
     gl.textureId = loadImageIntoGPU(wallpath, &gl.img_w , &gl.img_h, gl.textureId);
     cache_wallpaper(wallpath);
  }

  if (setupOpenGL(&wl, &gl)) {
    return 1;
  }

  // unix socket
  int daemon_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (daemon_sock == -1) {
    perror("Socket\n");
    return 1;
  }
  
  if (setupDaemonSocket(daemon_sock)) {
    return 1;
  }

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
    int timeout = wl.run ? 0 : -1;
    int ret = poll(fds, 2, timeout);

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
                   gl.speed = strtof(tok, NULL);
                   if (gl.speed <= 0.00) gl.speed = 0.00f;
                   if (gl.speed >= 1.00) gl.speed = 1.00f;
                 }
              }
              tok = strtok(NULL, " ");
            }

            if (path && strcmp(path, wallpath) != 0) {
              snprintf(wallpath, sizeof(wallpath), "%s",path);
              GLuint nexTex = loadImageIntoGPU(wallpath, &gl.img_w, &gl.img_h, gl.textureId);
              if (gl.textureId != nexTex) {
                if (gl.textureId != 0) {
                  glDeleteTextures(1, &gl.textureId);
                }
                gl.textureId = nexTex;
                cache_wallpaper(wallpath);
              }
              gl_draw(&wl, &gl);
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
      gl_draw(&wl, &gl);
    }
  }
  close(daemon_sock);
  unlink(SOCK_PATH);
  return 0;
}
