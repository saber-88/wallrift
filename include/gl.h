#pragma once

#include "monitor.h"
#include <GL/gl.h>
#include <GLES2/gl2.h>

typedef struct APP APP;
typedef struct GL{
  GLuint prog;
  GLuint vbo;
  GLuint vao;
  GLuint ebo;

  int cursorLoc;
  int imgWidthLoc;
  int imgHeightLoc;
  int oldImgWidthLoc;
  int oldImgHeightLoc;
  int viewWidthLoc;
  int viewHeightLoc;
  int old_tex_loc;
  int new_tex_loc;
  int progress_loc;
  int transition_loc;
  int transition_type;
  float speed;
} GL;

GLuint createShader(GLenum type, const char *shaderSrc); 

GLuint createProgram(const char *vFilePath, const char *fFilePath); 
GLuint loadImageIntoGPU(char *imgPath, int *imageWidth, int* imageHeight); 

void gl_draw(APP *app , Monitor *m);
int setupOpenGL(APP *app);
