#include "headers/log.h"

FILE *_log_file = 0;
bool init_logging(const char *log_file_path) {
    if (!(_log_file = fopen(log_file_path, "w+")))
    {
        fprintf(stderr, "Failed to open log file");
        return false;
    }
    return true;
}

void end_logging() {
    if (_log_file) {
        fclose(_log_file);
        _log_file = 0;
    }
}

void _print_stacktrace() {
    const int BT_BUFFER_SIZE = 255;
    void *bt[BT_BUFFER_SIZE];
    int nbt = backtrace(bt, BT_BUFFER_SIZE);
    char **symbols = backtrace_symbols(bt, BT_BUFFER_SIZE);
    printf("Stack Trace:\n");
    for (int i = 0; i < nbt; i++)
    {
        printf("\t%s\n", symbols[i]);
    }
    free(symbols);
    exit(EXIT_FAILURE);
}

bool enable_stacktrace() {
    signal(SIGILL, _print_stacktrace);
    signal(SIGABRT, _print_stacktrace);
    signal(SIGFPE, _print_stacktrace);
    signal(SIGSEGV, _print_stacktrace);
    signal(SIGTERM, _print_stacktrace);
    return true;
}