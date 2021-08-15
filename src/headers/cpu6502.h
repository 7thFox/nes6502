#ifndef CPU6502_H
#define CPU6502_H

#include "common.h"
#include "memmap.h"

typedef enum {
    STAT_N_NEGATIVE     = 0x80,
    STAT_V_OVERFLOW     = 0x40,
    STAT___IGNORE       = 0x20,
    STAT_B_BREAK        = 0x10,
    STAT_D_DECIMAL      = 0x08,
    STAT_I_INTERRUPT    = 0x04,
    STAT_Z_ZERO         = 0x02,
    STAT_C_CARRY        = 0x01,
} StatusFlags;

typedef struct {
    u8 ir;// Instruction Register
    u8 tcu;// Timing Control Unit
    u16 pc;
    u8 x;
    u8 y;
    u8 a;
    u8 sp;
    u8 p;// Flags

    // internal states/registers
    u8 pd;
    u16 addr_last_cycle;

    // 0 RW
    // 1 NMI
    // 2 IRQ
    u8 bit_fields;

    memaddr addr_bus;
    u8 data_bus;
    MemoryMap *memmap;

    void* (*on_next_clock)(void*);
} Cpu6502;

void cpu_pulse(Cpu6502 *c);
void cpu_resb(Cpu6502 *c);

static inline bool cpu_is_read(Cpu6502 *c) { return (c->bit_fields & (1 << 0)) == (1 << 0); }

#endif