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

typedef enum {
    PIN_READ = 0x1,
    PIN_NMI  = 0x2,
    PIN_IRQ  = 0x4,
} CpuSinglePins;

typedef struct {
    u8 ir;  // Instruction Register
    u8 tcu; // Timing Control Unit
    u16 pc; // Program Counter
    u8 x;
    u8 y;
    u8 a;   // Accumulator
    u8 sp;  // Stack Pointer
    u8 p;   // Status Flags
    u8 pd;  // Input Data Latch

    u8 bit_fields;

    memaddr addr_bus;
    u8 data_bus;
    MemoryMap *memmap;

    void* (*on_next_clock)(void*);
} Cpu6502;

void cpu_pulse(Cpu6502 *c);
void cpu_resb(Cpu6502 *c);

#endif