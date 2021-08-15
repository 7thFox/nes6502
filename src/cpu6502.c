#include "headers/cpu6502.h"

void _cpu_read(Cpu6502 *c) {
    c->pd = mem_read_addr(c->memmap, c->addr_last_cycle);
}

void *_cpu_fetch_opcode(Cpu6502 *c);
void *_cpu_fetch_lo(Cpu6502 *c);
void *_cpu_fetch_hi(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);
// void _cpu_fetch(Cpu6502 *c);

void cpu_pulse(Cpu6502 *c) {
    c->tcu++;
    c->addr_last_cycle = c->addr_bus;
    c->on_next_clock = (c->on_next_clock)(c);
}

void cpu_resb(Cpu6502 *c) {
    c->p |= 1 << 5;    // ?
    c->p |= 1 << 3;    // B
    c->p &= ~(1 << 3); // D
    c->p |= 1 << 2;    // I
    c->bit_fields |= 1 << 0;
    // we skip the whole 2-cycle set pc part (for now anyway)
    // by hacky coincidence, not defining this sets it to $0000 which is how I set up the rom for testing
    uint8_t lo = mem_read_addr(c->memmap, 0xfffc);
    uint8_t hi = mem_read_addr(c->memmap, 0xfffd);
    // c->pc = (hi << 8) | lo;
    c->pc = (hi << 8) | lo;
    c->addr_bus = c->pc;
    c->tcu = 0;
    c->on_next_clock = (void *(*)(void *))(_cpu_fetch_opcode);
    logf("Reset CPU. PC set to $%04x ($fffc: $%02x, $fffd: $%02x)\n", c->pc, lo, hi);
}

void* _cpu_fetch_opcode(Cpu6502 *c) {
    _cpu_read(c);
    c->ir = c->pd;
    c->addr_bus++;
    return _cpu_fetch_lo;
}
void* _cpu_fetch_lo(Cpu6502 *c) {
    _cpu_read(c);
    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    // tracef("_cpu_fetch_lo ir: $%02X (a: %i b: %i c: %i) lo: $%02X (%i) \n",
    //                           c->ir, op_a, op_b, op_c,        lo,  lo);

    switch (op_c)
    {
        case 0b00:
        {
            switch (op_b)
            {
                case 0:
                case 1:// zpg
                case 2:// impl
                {
                    switch (op_a)
                    {
                        case 0:// PHP
                        case 1:// PLP
                        case 2:// PHA
                        case 3:// PLA
                        case 4:// DEY
                        case 5:// TAY
                        case 6:// INY
                        case 7:// INX
                            c->x++;
                            break;
                    }
                    c->tcu = 0;
                    c->pc++;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                }
                case 3:// abs
                    c->addr_bus++;
                    return _cpu_fetch_hi;
                case 4: // rel
                case 5:// zpg,X
                case 6:// impl
                case 7:// abs,X
            }
            break;
        }
        case 0b01:
        {
            switch (op_b)
            {
                case 0:// X,ind
                case 1:// zpg
                case 2:// imm
                case 3:// abs
                case 4:// ind,Y
                case 5:// zpg,X
                case 6:// abs,Y
                case 7:// abs,X
            }
            break;
        }
        case 0b10:// 101 000 10
        {
            switch (op_b)
            {
                case 0:
                {
                    if (op_a == 5) { // LDX
                        c->x = c->pd;
                        c->tcu = 0;
                        c->pc += 2;
                        c->addr_bus = c->pc;
                        return _cpu_fetch_opcode;
                    }
                }
                case 1:// zpg
                case 2:// imm
                case 3:// abs
                case 4:// ind,Y
                case 5:// zpg,X
                case 6:// abs,Y
                case 7:// abs,X
            }
            break;
        }
    }

    return c->on_next_clock;
}

void* _cpu_fetch_hi(Cpu6502 *c) {
    c->addr_bus = c->pd;
    _cpu_read(c);
    c->addr_bus |= c->pd << 8;

    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;
    switch (op_b)
    {
        case 3: // abs
            switch (op_c)
            {
                case 0:
                    switch (op_a)
                    {
                        case 1: // BIT
                        case 2: // JMP
                            c->pc = c->addr_bus;
                            c->tcu = 0;
                            return _cpu_fetch_opcode;
                        case 3: // JMP ind
                        case 4: // STY
                        case 5: // LDY
                        case 6: // CPY
                        case 7: // CPX
                    }
                    break;
                case 1:
                case 2:
                    break;
            }
            break;
        case 6: // abs,Y
        case 7: // abs,X
            if (op_a == 5)// LDX abs,Y
            {

            }
            break;
    }
    return c->on_next_clock;
}