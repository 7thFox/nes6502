#ifndef PROFILE_H
#define PROFILE_H

#include "x86intrin.h"
#include "cpuid.h"
#include "execinfo.h"
#include "stdio.h"

long get_frame(void *fn);
void init_profiler();
void end_profiler(const char *file_name);
void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));

#endif