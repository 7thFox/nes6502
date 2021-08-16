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

void mem_add_ppu(MemoryMap *m, PPURegisters *p) {
    m->_ppu = p;// :(
}

MemoryBlock *mem_get_read_block(MemoryMap *m, memaddr addr) {
    tracef("mem_get_read_block $%04x\n", addr);

    for (uint i = 0; i < m->n_read_blocks; i++) {
        MemoryBlock *b = m->read_blocks + i;
        tracef("use %s? (%04x-%04x) ", b->block_name, b->range_low, b->range_high);
        if (b->range_low <= addr && b->range_high >= addr) {
            tracef("YES!\n");
            return b;
        }
        tracef("no\n");
    }
    return 0;
}

MemoryBlock *mem_get_write_block(MemoryMap *m, memaddr addr) {
    tracef("mem_get_write_block \n");

    for (uint i = 0; i < m->n_write_blocks; i++) {
        MemoryBlock *b = m->write_blocks + i;
        if (b->range_low <= addr && b->range_high >= addr) {
            return b;
        }
    }
    return 0;
}

u8 mem_read_addr(MemoryMap *m, memaddr addr) {
    tracef("mem_read_addr \n");

    if (m->_ppu && addr >= 0x2000 && addr <= 0x3FFF) {
        switch (addr % 0x08)
        {
            case 1: return m->_ppu->mask;
            case 2: return m->_ppu->status;
            case 4: return m->_ppu->oam_data;
            case 7: return m->_ppu->data;
        }
        return 0; // write-only
    }

    MemoryBlock *b = mem_get_read_block(m, addr);
    return b ? b->values[addr - b->range_low] : 0;
}

void mem_write_addr(MemoryMap *m, memaddr addr, u8 value) {
    tracef("mem_write_addr \n");

    if (m->_ppu && addr >= 0x2000 && addr <= 0x3FFF) {
        switch (addr % 0x08)
        {
            case 0: m->_ppu->controller = value; break;
            case 3: m->_ppu->oam_address = value; break;
            case 4: m->_ppu->oam_data = value; break;
            case 5: m->_ppu->scroll = value; break;
            case 6: m->_ppu->address = value; break;
            case 7: m->_ppu->data = value; break;
        }
        return; // read-only
    }

    MemoryBlock *b = mem_get_write_block(m, addr);
    if (b) b->values[addr - b->range_low] = value;
}