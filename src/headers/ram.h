#ifndef RAM_H
#define RAM_H

#include "common.h"
#include "stdio.h"
#include "stdlib.h"

typedef struct {
    memaddr map_offset;
    size_t  size;
    u8 *    value;
} Ram;

// bool ram_preload(Ram *rom, const char *filepath);

#endif