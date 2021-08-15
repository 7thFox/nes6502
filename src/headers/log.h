#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_MINIMAL 1
#define LOG_LEVEL_NORMAL 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_TRACE 4

#define LOG_LEVEL LOG_LEVEL_TRACE
// #define LOG_LEVEL LOG_LEVEL_DEBUG

#include "stdio.h"

// Define and set in entrypoint
extern FILE *log_file;

// #define LOG_FILE_TO_USE stdout
#define LOG_FILE_TO_USE log_file

#if LOG_LEVEL >= LOG_LEVEL_NORMAL
#define logf(...)              \
    fprintf(LOG_FILE_TO_USE, "[INFO] ");   \
    fprintf(LOG_FILE_TO_USE, __VA_ARGS__); \
    fflush(LOG_FILE_TO_USE);
#else
#define logf(...) ;
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define debugf(...)            \
    fprintf(LOG_FILE_TO_USE, "[DEBUG] ");  \
    fprintf(LOG_FILE_TO_USE, __VA_ARGS__); \
    fflush(LOG_FILE_TO_USE);
#else
#define debugf(...) ;
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define tracef(...)            \
    fprintf(LOG_FILE_TO_USE, "[TRACE] ");  \
    fprintf(LOG_FILE_TO_USE, __VA_ARGS__); \
    fflush(LOG_FILE_TO_USE);
#else
#define tracef(...) ;
#endif

#endif