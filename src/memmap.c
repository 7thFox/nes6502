#include "headers/memmap.h"

void mem_add_rom(MemoryMap *m, Rom *r) {
    if (r->rom_size > 0) {
        m->read_blocks[m->n_read_blocks].block_name = "RAM";
        m->read_blocks[m->n_read_blocks].range_low = r->map_offset;
        m->read_blocks[m->n_read_blocks].range_high = (memaddr)(r->map_offset + r->rom_size - 1);
        m->read_blocks[m->n_read_blocks].values = r->value;
        m->n_read_blocks++;
    }
}

MemoryBlock *mem_get_read_block(MemoryMap *m, memaddr addr) {
    for (int i = 0; i < m->n_read_blocks; i++) {
        MemoryBlock *b = m->read_blocks + i;
        if (b->range_low <= addr && b->range_high >= addr) {
            return b;
        }
    }
    return 0;
}

MemoryBlock *mem_get_write_block(MemoryMap *m, memaddr addr) {
    for (int i = 0; i < m->n_write_blocks; i++) {
        MemoryBlock *b = m->write_blocks + i;
        if (b->range_low <= addr && b->range_high >= addr) {
            return b;
        }
    }
    return 0;
}