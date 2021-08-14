#ifndef DISASM_H
#define DISASM_H

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "instruction.h"
#include "log.h"

typedef struct {
#define N_MAX_DISASM 64
#define N_MAX_TEXT_SIZE 20// "OPC $LLHH,X (65535)" = 19 + \0
#define N_MAX_BYTE_SIZE 10// "00 00 00 "     = 9 + \0
    char    _disasm_text[N_MAX_DISASM][N_MAX_TEXT_SIZE];
    uint8_t _disasm_offsets[N_MAX_DISASM];// N_MAX_DISASM <= 85
    char    _disasm_bytes[N_MAX_DISASM][N_MAX_BYTE_SIZE];
} Disassembler;

typedef struct {
    int count;
    char (*text)[N_MAX_TEXT_SIZE];
    char (*bytes)[N_MAX_BYTE_SIZE];
    uint8_t *offsets;
} Disassembly;

Disassembler *create_disassembler();

// You CANNOT use disassembly results after a subsequent call without copying the string arrays.
Disassembly disasm(Disassembler *d, uint8_t *data_aligned, size_t data_size, int n);

uint16_t disasm_get_alignment(Disassembler *d, uint16_t offset, int backtrack);

#endif