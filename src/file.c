#include "../include/file.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *readFile(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    LOG_ERR ("FILE",  "Failed to open shader file: %s", path);
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  char *data = (char *)malloc(size + 1);
  int read_bytes = fread(data, 1, size, f);
  if (read_bytes != size) {

    LOG_ERR ("FILE",  "Failed to read file completely");
    free(data);
    fclose(f);
    return NULL;
  }
  data[size] = '\0';

  fclose(f);
  return data;
}


const char* get_cache_file(void){
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
  LOG_INFO("FILE",  "Cached wallpaper: %s", wallpaper);
}

const char* get_cached_wallpaper(void) {
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

