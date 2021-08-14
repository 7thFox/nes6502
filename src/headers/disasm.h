#ifndef DISASM_H
#define DISASM_H

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "instruction.h"
#include "log.h"

typedef struct {
#define N_MAX_DISASM 16
#define N_MAX_TEXT_SIZE 12// "OPC $LLHH,X" = 11 + \0
#define N_MAX_BYTE_SIZE 9// "00 00 00"     = 8 + \0
    //  char _disasm_text_buffer[N_MAX_DISASM * N_MAX_TEXT_SIZE];
    //  char _disasm_bytes_buffer[N_MAX_DISASM * N_MAX_BYTE_SIZE];
     char _disasm_text[N_MAX_DISASM][N_MAX_TEXT_SIZE];
    //  char _disasm_bytes[N_MAX_DISASM][N_MAX_BYTE_SIZE];
// #define N_CACHED_ALIGNMENTS 16
     //     uint16_t sparse_alignments[N_CACHED_ALIGNMENTS];
     //     uint16_t alignments[N_CACHED_ALIGNMENTS];
} Disassembler;

typedef struct {
    int count;
    char (*text)[N_MAX_TEXT_SIZE];
    char (*bytes)[N_MAX_BYTE_SIZE];
} Disassembly;

Disassembler *create_disassembler();

// You CANNOT use disassembly results after a subsequent call without copying the
// string arrays.
Disassembly disasm(Disassembler *d, uint8_t *data_aligned, size_t data_size, int n);

uint16_t disasm_get_alignment(Disassembler *d, uint16_t offset, int backtrack);

#endif