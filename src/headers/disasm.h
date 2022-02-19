#ifndef DISASM_H
#define DISASM_H

#include "common.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

typedef struct {
#define N_MAX_DISASM    64
#define N_MAX_TEXT_SIZE 20 // "OPC $LLHH,X (65535)" = 19 + \0
#define N_MAX_BYTE_SIZE 10 // "00 00 00 "     = 9 + \0
    char _disasm_text[N_MAX_DISASM][N_MAX_TEXT_SIZE];
    u8   _disasm_offsets[N_MAX_DISASM]; // N_MAX_DISASM <= 85
    char _disasm_bytes[N_MAX_DISASM][N_MAX_BYTE_SIZE];
} Disassembler;

typedef struct {
    int count;
    char (*text)[N_MAX_TEXT_SIZE];
    char (*bytes)[N_MAX_BYTE_SIZE];
    u8 *offsets;
} Disassembly;

Disassembler *create_disassembler();

// You CANNOT use disassembly results after a subsequent call without copying the string arrays.
Disassembly disasm(Disassembler *d, u8 *data_aligned, size_t data_size, int n);

u16 disasm_get_alignment(Disassembler *d, u16 offset, int backtrack);

#endif