#pragma once

char *read_file(const char *path);
const char* get_cache_file(void);
void cache_wallpaper(const char* wallpaper);
const char* get_cached_wallpaper(void);

