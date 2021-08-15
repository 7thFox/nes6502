#ifndef RAM_H
#define RAM_H

#include "memaddr.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "log.h"

typedef struct {
    memaddr map_offset;
    size_t  size;
    uint8_t *value;
} Ram;

// bool ram_preload(Ram *rom, const char *filepath);

#endif