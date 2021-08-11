#ifndef ROM_H
#define ROM_H

#include "memmap.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"

typedef struct {
    memaddr map_offset;
    size_t  rom_size;
    uint8_t *value;
} Rom;

bool rom_load(Rom *rom, const char *filepath);

#endif