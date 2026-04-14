#include "../include/file.h"

char *readFile(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("[ERR][FILE]: Failed to open file\n");
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  char *data = (char *)malloc(size + 1);
  size_t read_bytes = fread(data, 1, size, f);
  if (read_bytes != size) {
    printf("[ERR][FILE]: failed to read file completely\n");
    free(data);
    fclose(f);
    return NULL;
  }
  data[size] = '\0';

  fclose(f);
  return data;
}


const char* get_cache_file(){
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
}

const char* get_cached_wallpaper() {
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

