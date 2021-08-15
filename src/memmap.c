#include "headers/memmap.h"

void mem_add_rom(MemoryMap *m, Rom *r, const char *name) {
    tracef("mem_add_rom \n");
    if (r->rom_size > 0) {
        m->read_blocks[m->n_read_blocks].block_name = name;
        m->read_blocks[m->n_read_blocks].range_low = r->map_offset;
        m->read_blocks[m->n_read_blocks].range_high = (memaddr)(r->map_offset + r->rom_size - 1);
        m->read_blocks[m->n_read_blocks].values = r->value;
        m->n_read_blocks++;
    }
}

void mem_add_ram(MemoryMap *m, Ram *r, const char *name) {
    tracef("mem_add_ram \n");
    if (r->size > 0) {
        m->read_blocks[m->n_read_blocks].block_name = name;
        m->read_blocks[m->n_read_blocks].range_low = r->map_offset;
        m->read_blocks[m->n_read_blocks].range_high = (memaddr)(r->map_offset + r->size - 1);
        m->read_blocks[m->n_read_blocks].values = r->value;
        m->n_read_blocks++;

        m->write_blocks[m->n_write_blocks].block_name = name;
        m->write_blocks[m->n_write_blocks].range_low = r->map_offset;
        m->write_blocks[m->n_write_blocks].range_high = (memaddr)(r->map_offset + r->size - 1);
        m->write_blocks[m->n_write_blocks].values = r->value;
        m->n_write_blocks++;
    }
}

MemoryBlock *mem_get_read_block(MemoryMap *m, memaddr addr) {
    tracef("mem_get_read_block \n");
    for (int i = 0; i < m->n_read_blocks; i++) {
        MemoryBlock *b = m->read_blocks + i;
        if (b->range_low <= addr && b->range_high >= addr) {
            return b;
        }
    }
    return 0;
}

MemoryBlock *mem_get_write_block(MemoryMap *m, memaddr addr) {
    tracef("mem_get_write_block \n");
    for (int i = 0; i < m->n_write_blocks; i++) {
        MemoryBlock *b = m->write_blocks + i;
        if (b->range_low <= addr && b->range_high >= addr) {
            return b;
        }
    }
    return 0;
}

uint8_t mem_read_addr(MemoryMap *m, memaddr addr) {
    tracef("mem_read_addr \n");
    MemoryBlock *b = mem_get_read_block(m, addr);
    return b ? b->values[addr - b->range_low] : 0;
}

void mem_write_addr(MemoryMap *m, memaddr addr, uint8_t value) {
    tracef("mem_write_addr \n");
    MemoryBlock *b = mem_get_write_block(m, addr);
    if (b) b->values[addr - b->range_low] = value;
}