#ifndef MEMMAP_H
#define MEMMAP_H

#include "stdio.h"

#include "common.h"
#include "ppuregisters.h"
#include "ram.h"
#include "rom.h"

typedef u16 memaddr;

typedef struct {
    const char *block_name;
    memaddr range_low;
    memaddr range_high;
    u8 *values;
} MemoryBlock;

typedef struct {
#define MEM_MAP_MAX_BLOCKS 16
    uint n_read_blocks;
    uint n_write_blocks;
    MemoryBlock read_blocks[MEM_MAP_MAX_BLOCKS];
    MemoryBlock write_blocks[MEM_MAP_MAX_BLOCKS];
    PPURegisters *_ppu;// I hate that I need to do this, but PPU really f's up my mapping
} MemoryMap;

void mem_add_rom(MemoryMap *m, Rom *r, const char *name);
void mem_add_ram(MemoryMap *m, Ram *r, const char *name);
void mem_add_ppu(MemoryMap *m, PPURegisters *p);

// use for debug purposes only; not always accurate
MemoryBlock *mem_get_read_block(MemoryMap *m, memaddr addr);
// use for debug purposes only; not always accurate
MemoryBlock *mem_get_write_block(MemoryMap *m, memaddr addr);

u8 mem_read_addr(MemoryMap *m, memaddr addr);
void mem_write_addr(MemoryMap *m, memaddr addr, u8 value);

#endif