/*
 author : github.com/saber-88
 */

#include <fcntl.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include "../include/wayland.h"
#include "../include/gl.h"
#include "../include/file.h"
#include "log.h"
#include "../include/app.h"
#include "monitor.h"

#define SOCK_PATH "/tmp/wallrift.sock"

//forward declarations
int setup_daemon_socket(void);
void handle_client(int daemon_sock, APP *app);
void load_wallpaper_for_monitor(APP *app, Monitor* m, const char *path);


int setup_daemon_socket(void){
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
    LOG_ERR ("SOCK",  "Failed to bind socket");
    close(daemon_sock);
    return -1;
  }

  LOG_INFO("SOCK",  "Socket created at %s", SOCK_PATH);
  listen(daemon_sock, 5);

  LOG_INFO("SOCK",  "Listening...");
  int flags = fcntl(daemon_sock, F_GETFL, 0);
  fcntl(daemon_sock, F_SETFL, flags | O_NONBLOCK);
  return daemon_sock;
}
void load_wallpaper_for_monitor(APP *app, Monitor *m, const char *path) {
  if (!path || path[0] == '\0') return;

  eglMakeCurrent(app->egl.egl_display, m->egl_surface, m->egl_surface, app->egl.egl_context);

  int new_w = 0, new_h = 0;
  GLuint nextTex = loadImageIntoGPU((char*)path, &new_w, &new_h, m->textureId);

  if (new_w == 0 || new_h == 0) {
    LOG_ERR("GL", "Failed to load image: %s", path);
    return;
  }

  if (m->textureId != 0 && m->textureId != nextTex) {
    glDeleteTextures(1, &m->textureId);
  }
  m->textureId = nextTex;
  m->img_w = new_w;
  m->img_h = new_h;
  snprintf(m->wallpath, sizeof(m->wallpath), "%s", path); 
}

void handle_client(int daemon_sock, APP *app){
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

      if (path) {
        Monitor *m = app->active_monitor;
        if (!m){
          LOG_WARN("DAEMON", "No active monitor");
          return;
        } 
        if (strcmp(path, m->wallpath) == 0) return; 
        load_wallpaper_for_monitor(app, m, path);
        gl_draw(app, app->active_monitor);
        cache_wallpaper(path);
      }
    }
  }
}

int main(void) {
  
  APP *app = calloc(1, sizeof(APP));

  /* setting initial speed to 0.05 for smooth parallax
   * i need to implement config file support soon so 
   * that i can read from config file instead of 
   * hardcoding values.
   */
  app->gl.speed = 0.05f;

  setupWayland(app);
  setupCursor(app);
  setupEGLGlobal(app);
  
  // setting up surface and egl for each monitor
  for (int i = 0; i < app->monitor_count; i++) {
    setupSurface(app, &app->monitors[i]);
    setupEGL(app, &app->monitors[i]);
    LOG_INFO("DAEMON", "Setup done for monitor %d with id: %d",i,app->monitors[i].global_name);
  }

  /* making context current before opengGL setup
   * because nvidia likes it this way
   */

  eglMakeCurrent(app->egl.egl_display,
                 app->monitors[0].egl_surface,
                 app->monitors[0].egl_surface,
                 app->egl.egl_context
                );
  
  // setting up OpenGL
  if (setupOpenGL(app)) {
    LOG_ERR("DAEMON","Failed to setup opengGL");
    return 1;
  }
  
  app->gl.texLoc = glGetUniformLocation(app->gl.prog, "tex");

  
  // initially loading cached wallpaper for every monitor
  const char* cached = get_cached_wallpaper();
  if (cached) {
     for (int i = 0; i < app->monitor_count; i++) {
       load_wallpaper_for_monitor(app, &app->monitors[i], cached);
    }   
  }

  
  // initially drawing cached wallpaper for every monitor
  for (int i = 0; i < app->monitor_count; i++) {
    gl_draw(app, &app->monitors[i]);
  }
  wl_display_flush(app->wl.display);

  int daemon_sock = setup_daemon_socket();
  if (daemon_sock == -1) {
    LOG_ERR("SOCK","Failed to setup daemon socket");
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
    int timeout = -1;
    if (app->active_monitor && app->active_monitor->pointer_inside) {
      if (app->wl.cursor_moved) {
        timeout = 0;
      }
    }
    int ret = poll(fds, 2, timeout);

    if (ret > 0) {
      // wayland event happend
      if (fds[0].revents & POLLIN) {
        wl_display_read_events(app->wl.display);
        did_read = 1;
      }
      // socket event happend
      if (fds[1].revents & POLLIN) {
        handle_client(daemon_sock, app);
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
