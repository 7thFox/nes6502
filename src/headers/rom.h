#ifndef ROM_H
#define ROM_H

#include "common.h"
#include "stdio.h"
#include "stdlib.h"

typedef struct {
    memaddr map_offset;
    size_t  rom_size;
    u8 *    value;
} Rom;

bool rom_load(Rom *rom, const char *filepath);

#endif