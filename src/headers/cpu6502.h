#ifndef CPU6502_H
#define CPU6502_H

#include "stdint.h"
#include "memmap.h"

typedef struct {
    uint8_t ir;// Instruction Register
    uint8_t tcu;// Timing Control Unit
    uint8_t pch;
    uint8_t pcl;
    uint8_t s;
    uint8_t x;
    uint8_t y;
    uint8_t a;
    uint8_t p;// Flags

    // 0 RW
    // 1 NMI
    // 2 IRQ
    uint8_t bit_fields;

    memaddr addr_bus;
    uint8_t data_bus;
    MemoryMap *memmap;
} Cpu6502;

void cpu_pulse(Cpu6502 *c);
void cpu_resb(Cpu6502 *c);

static inline bool cpu_is_read(Cpu6502 *c) { return c->bit_fields & (1 << 0) == (1 << 0); }

#endif