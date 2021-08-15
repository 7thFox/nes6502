#include "headers/cpu6502.h"

#define setflag(p, f) p |= f
#define unsetflag(p, f) p &= ~(f);
#define setunsetflag(p, f, c) \
    if (c)                    \
        setflag(p, f);        \
    else                      \
        unsetflag(p, f);

void _cpu_update_p_flags(Cpu6502 *c, uint8_t val, StatusFlags f) {
    if (f & STAT_N_NEGATIVE) {
        setunsetflag(c->p, STAT_N_NEGATIVE, val & 0x80);
    }
    if (f & STAT_V_OVERFLOW) {
        setunsetflag(c->p, STAT_V_OVERFLOW, val & 0x80);
    }
    if (f & STAT_Z_ZERO) {
        setunsetflag(c->p, STAT_Z_ZERO, val == 0x00);
    }
}

void _cpu_read(Cpu6502 *c) {
    c->pd = mem_read_addr(c->memmap, c->addr_last_cycle);
}

void *_cpu_fetch_opcode(Cpu6502 *c);
void *_cpu_fetch_lo(Cpu6502 *c);
void *_cpu_fetch_hi(Cpu6502 *c);
void *_cpu_read_addr(Cpu6502 *c);
void *_cpu_page_boundary(Cpu6502 *c);
void *_cpu_read_addr_ind(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);

void cpu_pulse(Cpu6502 *c) {
    tracef("cpu_pulse \n");
    c->tcu++;
    c->addr_last_cycle = c->addr_bus;
    c->on_next_clock = (c->on_next_clock)(c);
}

void cpu_resb(Cpu6502 *c) {
    tracef("cpu_resb \n");
    setflag(c->p, STAT___IGNORE | STAT_B_BREAK | STAT_I_INTERRUPT);
    unsetflag(c->p, STAT_D_DECIMAL);
    // we skip the whole 2-cycle set pc part (for now anyway)
    // by hacky coincidence, not defining this sets it to $0000 which is how I set up the rom for testing
    uint8_t lo = mem_read_addr(c->memmap, 0xfffc);
    uint8_t hi = mem_read_addr(c->memmap, 0xfffd);
    // c->pc = (hi << 8) | lo;
    c->pc = (hi << 8) | lo;
    c->addr_bus = c->pc;
    c->tcu = 0;
    c->bit_fields |= 0x1;
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
        case 0:
        {
            switch (op_b)
            {
                case 0:
                    break;
                case 1: // zpg
                    break;
                case 2: // impl
                    switch (op_a)
                    {
                        case 0: // PHP
                        case 1: // PLP
                        case 2: // PHA
                        case 3: // PLA
                        case 4: // DEY
                        case 5: // TAY
                        case 6: // INY
                        case 7: // INX
                            c->x++;
                            _cpu_update_p_flags(c, c->x, STAT_N_NEGATIVE | STAT_Z_ZERO);
                            break;
                    }
                    c->tcu = 0;
                    c->pc++;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 3: // abs
                    c->addr_bus++;
                    return _cpu_fetch_hi;
                case 4: // rel
                {
                    uint16_t pc_before = c->pc;
                    switch (op_a)
                    {
                        case 0: // BPL
                            if ((c->p & STAT_N_NEGATIVE) == 0) {
                                c->pc += c->pd;
                            }
                            break;
                        case 1: // BMI
                            break;
                        case 2: // BVC
                            break;
                        case 3: // BVS
                            break;
                        case 4: // BCC
                            break;
                        case 5: // BCS
                            break;
                        case 6: // BNE
                            break;
                        case 7: // BEQ
                            break;
                    }
                    if ((pc_before & 0xF0) != (c->pc & 0xF0)) {
                        return _cpu_page_boundary;
                    }
                    c->addr_bus = c->pc;
                    c->tcu = 0;
                    return _cpu_fetch_opcode;
                }
                case 5: // zpg,X
                    break;
                case 6: // impl
                    switch (op_a)
                    {
                        case 0: // CLC
                            break;
                        case 1: // SEC
                            break;
                        case 2: // CLI
                            break;
                        case 3: // SEI
                            setflag(c->p, STAT_I_INTERRUPT);
                            break;
                        case 4: // TYA
                            break;
                        case 5: // CLV
                            break;
                        case 6: // CLD
                            unsetflag(c->p, STAT_D_DECIMAL);
                            break;
                        case 7: // SED
                            break;
                    }
                    c->tcu = 0;
                    c->pc++;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 7:// abs,X
                    break;
            }
            break;
        }
        case 1:
        {
            switch (op_b)
            {
                case 0: // X,ind
                    break;
                case 1: // zpg
                    break;
                case 2: // imm
                    break;
                case 3: // abs
                    c->addr_bus++;
                    return _cpu_fetch_hi;
                case 4: // ind,Y
                    break;
                case 5: // zpg,X
                    break;
                case 6: // abs,Y
                    break;
                case 7: // abs,X
                    break;
            }
            break;
        }
        case 2:
        {
            switch (op_b)
            {
                case 0: // imm
                    if (op_a == 5) { // LDX
                        c->x = c->pd;
                        c->tcu = 0;
                        c->pc += 2;
                        c->addr_bus = c->pc;
                        _cpu_update_p_flags(c, c->x, STAT_N_NEGATIVE | STAT_Z_ZERO);
                        return _cpu_fetch_opcode;
                    }
                    break;
                case 1: // zpg
                    break;
                case 2: // impl
                    break;
                case 3: // abs
                    c->addr_bus++;
                    return _cpu_fetch_hi;
                //   4
                case 5: // zpg,X zpg,Y
                    break;
                case 6: // impl
                    switch (op_a)
                    {
                        case 4: // TXS
                            c->sp = c->x;
                            break;
                        case 5: // TSX
                            break;
                    }
                    c->tcu = 0;
                    c->pc++;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 7: // abs,X
                    break;
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

    // int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    // int op_c = (c->ir & 0b00000011) >> 0;


    switch (op_b)
    {
        // case 3: // abs
        case 6: // abs,Y
            break;
        case 7: // abs,X
            break;
    }

    if (c->ir == 0x4C) { // JMP
        c->pc = c->addr_bus;
        c->tcu = 0;
        return _cpu_fetch_opcode;
    }

    return _cpu_read_addr;
}

void *_cpu_read_addr(Cpu6502 *c) {
    _cpu_read(c);

    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    switch (op_c)
    {
        case 0:
            if (c->ir == 0x6c) { // ind (JMP)
                c->addr_bus++;
                return _cpu_read_addr_ind;
            }
            break;
        case 1:
            switch (op_b)
            {
                case 0: // X,ind
                    break;
                case 4: // ind,Y
                    break;
            }
            switch (op_a)
            {
                case 0: // ORA
                    break;
                case 1: // AND
                    break;
                case 2: // EOR
                    break;
                case 3: // ADC
                    break;
                case 4: // STA
                    break;
                case 5: // LDA
                    c->a = c->pd;
                    _cpu_update_p_flags(c, c->a, STAT_N_NEGATIVE | STAT_Z_ZERO);
                    break;
                case 6: // CMP
                    break;
                case 7: // SBC
                    break;
            }
            switch (op_b)
            {
                case 1: // zpg
                case 5: // zpg,X
                    c->pc += 2;
                    break;
                case 3: // abs
                case 6: // abs,Y
                case 7: // abs,X
                    c->pc += 3;
                    break;
            }
            c->tcu = 0;
            c->addr_bus = c->pc;
            return _cpu_fetch_opcode;
        case 2:
            switch (op_a)
            {
                case 0: // ASL
                    break;
                case 1: // ROL
                    break;
                case 2: // LSR
                    break;
                case 3: // ROR
                    break;
                case 4: // STX
                    break;
                case 5: // LDX
                    c->x = c->pd;
                    _cpu_update_p_flags(c, c->x, STAT_N_NEGATIVE | STAT_Z_ZERO);
                    break;
                case 6: // DEC
                    break;
                case 7: // INC
                    break;
            }
            switch (op_b)
            {
                case 1: // zpg
                case 5: // zpg,X
                    c->pc += 2;
                    break;
                case 3: // abs
                case 7: // abs,X
                    c->pc += 3;
                    break;
            }
            c->tcu = 0;
            c->addr_bus = c->pc;
            return _cpu_fetch_opcode;
    }
    return c->on_next_clock;
}

void *_cpu_page_boundary(Cpu6502 *c) {
    c->tcu = 0;
    c->addr_bus = c->pc;
    return _cpu_fetch_opcode;
}

void *_cpu_read_addr_ind(Cpu6502 *c) {
    return c->on_next_clock;
}
