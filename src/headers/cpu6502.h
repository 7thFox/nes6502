#ifndef CPU6502_H
#define CPU6502_H

#include "stdint.h"
#include "memmap.h"
#include "log.h"

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
    uint8_t ir;// Instruction Register
    uint8_t tcu;// Timing Control Unit
    uint16_t pc;
    uint8_t x;
    uint8_t y;
    uint8_t a;
    uint8_t sp;
    uint8_t p;// Flags

    // internal states/registers
    uint8_t pd;
    uint16_t addr_last_cycle;

    // 0 RW
    // 1 NMI
    // 2 IRQ
    uint8_t bit_fields;

    memaddr addr_bus;
    uint8_t data_bus;
    MemoryMap *memmap;

    void* (*on_next_clock)(void*);
} Cpu6502;

void cpu_pulse(Cpu6502 *c);
void cpu_resb(Cpu6502 *c);

static inline bool cpu_is_read(Cpu6502 *c) { return c->bit_fields & (1 << 0) == (1 << 0); }

#endif