#include "gl.h"
#include "log.h"
#include "monitor.h"
#include "wayland.h"
#include "file.h"
#include "app.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

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

GLuint createShader(GLenum type, const char *shaderSrc) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &shaderSrc, NULL);
  glCompileShader(shader);
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    LOG_ERR("GL", "Failed to compile shader: %s",infoLog);
    return 0;
  }
  return shader;
}

GLuint createProgram(const char *vFilePath, const char *fFilePath) {
  const char *vsrc = readFile(vFilePath);
  const char *fsrc = readFile(fFilePath);
  if (!vsrc || !fsrc) {
    LOG_ERR("GL", "Failed to open shader file");
    free((void*)vsrc);
    free((void*)fsrc);
    return 0;
  }

  GLuint vs = createShader(GL_VERTEX_SHADER, vsrc);
  GLuint fs = createShader(GL_FRAGMENT_SHADER, fsrc);
  GLint success;
  char infoLog[512];

  if (vs == 0 || fs == 0) {
    LOG_ERR("GL", "Failed to create shader.");
    free((void*)vsrc);
    free((void*)fsrc);
    return -1;
  }
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  glGetProgramiv(prog, GL_LINK_STATUS, &success);

  free((void *)vsrc);
  free((void *)fsrc);

  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if (success != GL_TRUE) {
    glGetProgramInfoLog(prog, 512, NULL, infoLog);
    LOG_ERR("GL", "Failed to link program: %s",infoLog);
  }
  glDetachShader(prog, vs);
  glDetachShader(prog, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}
GLuint loadImageIntoGPU(char *imgPath, int *imageWidth, int *imageHeight) {

  char expanded[1024];
  if (imgPath[0] == '~') {
    snprintf(expanded, sizeof(expanded), "%s%s", getenv("HOME"), &imgPath[1]);
  }
  else {
    snprintf(expanded, sizeof(expanded), "%s", imgPath);
  }
  int channels;

  unsigned char *pixels = stbi_load(expanded, imageWidth, imageHeight, &channels, 4);
  if (!pixels) {
    LOG_ERR("GL", "Failed to load image");
    return 0;
  }

  GLuint texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *imageWidth, *imageHeight, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(pixels);
  LOG_INFO("GL","%s loaded in GPU", expanded);
  LOG_INFO("GL",    "Image size: %dx%d", *imageWidth, *imageHeight);
  return texID;
}

void gl_draw(APP *app, Monitor *m){

    if (!eglMakeCurrent(app->egl.egl_display, m->egl_surface, m->egl_surface, app->egl.egl_context)) {
      LOG_ERR("GL", "eglMakeCurrent failed");
    }

    glViewport(0, 0, m->width, m->height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(app->gl.prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m->old_texture_id);
    glUniform1i(app->gl.old_tex_loc, 0);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m->new_texture_id);
    glUniform1i(app->gl.new_tex_loc, 1);

    glBindBuffer(GL_ARRAY_BUFFER, app->gl.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->gl.ebo);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);


    // update mouse constatnly for parallax
    m->cursor_x += (m->target_cursor - m->cursor_x) * app->gl.speed;

    glUniform1f(app->gl.cursorLoc, m->cursor_x);
    glUniform1f(app->gl.imgWidthLoc, (float)m->img_w);
    glUniform1f(app->gl.imgHeightLoc, (float)m->img_h);

    glUniform1f(app->gl.oldImgWidthLoc, (float)m->old_img_w);
    glUniform1f(app->gl.oldImgHeightLoc, (float)m->old_img_h);

    glUniform1f(app->gl.viewWidthLoc, (float)m->width);
    glUniform1f(app->gl.viewHeightLoc, (float)m->height);

    if (m->transition_required && m->in_transition) {
      // transition speed
      m->progress += 0.01f;

      if (m->progress >= 1.0f) {
        m->progress = 1.0f;
        m->in_transition = 0;
        glDeleteTextures(1, &m->old_texture_id);
        m->old_texture_id = 0;
      }
    }
    glUniform1f(app->gl.progress_loc, m->progress);
    glUniform1i(app->gl.transition_loc, app->gl.transition_type);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    eglSwapBuffers(app->egl.egl_display, m->egl_surface);
}

int setupOpenGL(APP *app){

  if ((app->gl.prog =
      createProgram("/home/karmveer/.coding/wallrift/shaders/wallpaper.vert",
                    "/home/karmveer/.coding//wallrift/shaders/wallpaper.frag")) == 0) {

    LOG_ERR("GL", "Failed to create program");
    return 1;
  }

  
  app->gl.cursorLoc = glGetUniformLocation(app->gl.prog, "u_cursor");
  app->gl.imgWidthLoc = glGetUniformLocation(app->gl.prog, "u_img_width");
  app->gl.imgHeightLoc = glGetUniformLocation(app->gl.prog, "u_img_height");

  app->gl.oldImgWidthLoc = glGetUniformLocation(app->gl.prog, "u_old_img_width");
  app->gl.oldImgHeightLoc = glGetUniformLocation(app->gl.prog, "u_old_img_height");

  app->gl.viewWidthLoc = glGetUniformLocation(app->gl.prog, "u_view_width");
  app->gl.viewHeightLoc = glGetUniformLocation(app->gl.prog, "u_view_height");
  app->gl.new_tex_loc = glGetUniformLocation(app->gl.prog, "u_new_tex");
  app->gl.old_tex_loc = glGetUniformLocation(app->gl.prog, "u_old_tex");
  app->gl.progress_loc = glGetUniformLocation(app->gl.prog, "u_progress");
  app->gl.transition_loc = glGetUniformLocation(app->gl.prog, "u_type");

  glUseProgram(app->gl.prog);
  glGenBuffers(1, &app->gl.vbo);
  glGenBuffers(1, &app->gl.ebo);
  
  glBindBuffer(GL_ARRAY_BUFFER, app->gl.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->gl.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  return 0;
}
