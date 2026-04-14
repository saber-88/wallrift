#include "../include/gl.h"

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
    fprintf(stderr,"[ERR][SHADER]: %s\n", infoLog);
    return -1;
  }
  return shader;
}

GLuint createProgram(const char *vFilePath, const char *fFilePath) {
  const char *vsrc = readFile(vFilePath);
  const char *fsrc = readFile(fFilePath);
  if (!vsrc || !fsrc) {
    fprintf(stderr,"[ERR][FILE]: %s\n", "Failed to open shader file.");
    free((void*)vsrc);
    free((void*)fsrc);
    return -1;
  }
  GLuint vs = createShader(GL_VERTEX_SHADER, vsrc);
  GLuint fs = createShader(GL_FRAGMENT_SHADER, fsrc);

  if (vs == -1 || fs == -1) {
    fprintf(stderr,"[ERR][SHADER]: %s\n", "Failed to create shader.");
    free((void*)vsrc);
    free((void*)fsrc);
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
    fprintf(stderr,"[ERR][LINKING] :%s\n", infoLog);
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
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
    fprintf(stderr,"\n[ERR][STB]: Cannot load image\n");
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

void gl_draw(WL *wl, GL *gl){
    
    glViewport(0, 0, wl->width, wl->height);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(gl->prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gl->textureId);
    glUniform1i(gl->texLoc, 0);

    // update mouse constatnly for parallax
    wl->cursor_x += (wl->target_cursor - wl->cursor_x) * gl->speed;
    glUniform1f(gl->cursorLoc, wl->cursor_x);
    glUniform1f(gl->imgWLoc, (float)gl->img_w);
    glUniform1f(gl->viewWLoc, (float)wl->width);
    glUniform1f(gl->imgHLoc, (float)gl->img_h);
    glUniform1f(gl->viewHLoc, (float)wl->height);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    eglSwapBuffers(wl->egl_display, wl->egl_surface);

}

int setupOpenGL(WL *wl, GL *gl){

  if ((gl->prog =
      createProgram("/usr/share/wallrift/shaders/wallpaper.vert",
                    "/usr/share/wallrift/shaders/wallpaper.frag")) == -1) {
    
    fprintf(stderr,"\n[ERR][GL]: Failed to create program.\n");
    return 1;
  }
  gl->cursorLoc = glGetUniformLocation(gl->prog, "u_cursor");
  gl->imgWLoc = glGetUniformLocation(gl->prog, "u_img_width");
  gl->imgHLoc = glGetUniformLocation(gl->prog, "u_img_height");
  gl->viewWLoc = glGetUniformLocation(gl->prog, "u_view_width");
  gl->viewHLoc = glGetUniformLocation(gl->prog, "u_view_height");
  
  glUseProgram(gl->prog);
  glGenBuffers(1, &gl->vbo);
  glGenBuffers(1, &gl->ebo);

  glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  int resLoc = glGetUniformLocation(gl->prog, "resolution");
  glUniform2f(resLoc, wl->width, wl->height);
  int imgLoc = glGetUniformLocation(gl->prog, "imgSize");
  glUniform2f(imgLoc, gl->img_w, gl->img_h);
  return 0;
}
