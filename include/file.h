#pragma once
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

char *readFile(const char *path);
const char* get_cache_file();
void cache_wallpaper(const char* wallpaper);
const char* get_cached_wallpaper();

