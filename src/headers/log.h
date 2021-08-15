#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_MINIMAL 1
#define LOG_LEVEL_NORMAL 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_TRACE 4

// #define LOG_LEVEL LOG_LEVEL_TRACE
#define LOG_LEVEL LOG_LEVEL_DEBUG

#include "stdio.h"

// Define and set in entrypoint
extern FILE *log_file;

#if LOG_LEVEL >= LOG_LEVEL_NORMAL
#define logf(...)              \
    fprintf(log_file, "[INFO] ");   \
    fprintf(log_file, __VA_ARGS__); \
    fflush(log_file);
#else
#define logf(...) ;
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define debugf(...)            \
    fprintf(log_file, "[DEBUG] ");  \
    fprintf(log_file, __VA_ARGS__); \
    fflush(log_file);
#else
#define debugf(...) ;
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define tracef(...)            \
    fprintf(log_file, "[TRACE] ");  \
    fprintf(log_file, __VA_ARGS__); \
    fflush(log_file);
#else
#define tracef(...) ;
#endif

#endif