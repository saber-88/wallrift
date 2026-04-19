#pragma once
#include <stdio.h>

// log levels
#define LOG_LEVEL_INFO  0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_ERR   2

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// colors
#define CLR_RESET  "\033[0m"
#define CLR_GREEN  "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_RED    "\033[31m"
#define CLR_CYAN   "\033[36m"
#define CLR_GRAY   "\033[90m"

#ifdef NDEBUG
  #define _LOG_LOCATION ""
#else
  #define _LOG_LOCATION CLR_GRAY " (%s:%d)" CLR_RESET, __FILE__, __LINE__
#endif

// public macros
#if LOG_LEVEL <= LOG_LEVEL_INFO
  #define LOG_INFO(subsystem, fmt, ...) \
    fprintf(stdout, CLR_GREEN "[INFO]" CLR_RESET "[" subsystem "]: " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_INFO(subsystem, fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
  #define LOG_WARN(subsystem, fmt, ...) \
    fprintf(stderr, CLR_YELLOW "[WARN]" CLR_RESET "[" subsystem "]: " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_WARN(subsystem, fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERR
  #define LOG_ERR(subsystem, fmt, ...) \
    fprintf(stderr, CLR_RED "[ERR]" CLR_RESET "[" subsystem "]: " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_ERR(subsystem, fmt, ...)
#endif
