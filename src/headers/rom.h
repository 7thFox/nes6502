#ifndef ROM_H
#define ROM_H

#include "stdlib.h"
#include "stdio.h"

#include "common.h"

typedef struct {
    memaddr map_offset;
    size_t  rom_size;
    u8 *value;
} Rom;

bool rom_load(Rom *rom, const char *filepath);

#endif