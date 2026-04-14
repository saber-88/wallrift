#pragma once

#include <GL/gl.h>
#include <GLES2/gl2.h>
#include "wayland.h"
#include "file.h"

typedef struct{
  GLuint prog;
  GLuint vbo;
  GLuint vao;
  GLuint ebo;
  GLuint textureId;

  int cursorLoc;
  int imgWLoc;
  int imgHLoc;
  int viewWLoc;
  int viewHLoc;
  int texLoc;
  int img_w;
  int img_h;
  float speed;
} GL;

GLuint createShader(GLenum type, const char *shaderSrc); 

GLuint createProgram(const char *vFilePath, const char *fFilePath); 
GLuint loadImageIntoGPU(char *imgPath, int *imageWidth, int* imageHeight, GLuint texID); 

void gl_draw(WL *wl, GL *gl);
int setupOpenGL(WL *wl, GL *gl);
