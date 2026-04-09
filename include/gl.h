#pragma once
#include <GL/gl.h>
#include <GLES2/gl2.h>

typedef struct{
  GLuint prog;
  GLuint vbo;
  GLuint vao;
  GLuint ebo;

  int cursorLoc;
  int imgWLoc;
  int imgHLoc;
  int viewWLoc;
  int viewHLoc;

} GL;
