#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_MINIMAL 1
#define LOG_LEVEL_NORMAL  2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_TRACE   4

// #define LOG_LEVEL LOG_LEVEL_TRACE
#define LOG_LEVEL LOG_LEVEL_DEBUG

#include "execinfo.h"
#include "signal.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"

// Define and set in entrypoint
extern FILE *_log_file;

bool init_logging(const char *log_file_path);
void end_logging();
bool enable_stacktrace();

// #define LOG_FILE_TO_USE stdout
#define LOG_FILE_TO_USE _log_file

#define _logf(prefix, ...)                     \
    if (LOG_FILE_TO_USE) {                     \
        fprintf(LOG_FILE_TO_USE, prefix);      \
        fprintf(LOG_FILE_TO_USE, __VA_ARGS__); \
        fflush(LOG_FILE_TO_USE);               \
    }

#if LOG_LEVEL >= LOG_LEVEL_NORMAL
#define logf(...) _logf("[INFO] ", __VA_ARGS__)
#else
#define logf(...) ;
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define debugf(...) _logf("[DEBUG] ", __VA_ARGS__)
#else
#define debugf(...) ;
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define tracef(...) _logf("[TRACE] ", __VA_ARGS__)
#else
#define tracef(...) ;
#endif

#endif