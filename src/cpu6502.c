#include "headers/cpu6502.h"

#define setflag(p, f) p |= f
#define unsetflag(p, f) p &= ~(f);
#define setunsetflag(p, f, c) \
    if (c)                    \
        setflag(p, f);        \
    else                      \
        unsetflag(p, f);

void _cpu_update_NZ_flags(Cpu6502 *c, u8 val) {
    setunsetflag(c->p, STAT_N_NEGATIVE, val & 0x80);
    setunsetflag(c->p, STAT_Z_ZERO, val == 0x00);
}

void *_cpu_fetch_opcode(Cpu6502 *c);
void *_cpu_fetch_lo(Cpu6502 *c);
void *_cpu_fetch_hi(Cpu6502 *c);
void *_cpu_read_addr(Cpu6502 *c);
void *_cpu_page_boundary(Cpu6502 *c);
void *_cpu_read_addr_ind(Cpu6502 *c);
void *_cpu_push(Cpu6502 *c);
void *_cpu_pop(Cpu6502 *c);
void *_cpu_page_boundray(Cpu6502 *c);
void *_cpu_write_then_fetch_add3(Cpu6502 *c);
void *_cpu_write_brk_write_pclo(Cpu6502 *c);
void *_cpu_write_brk_write_sr(Cpu6502 *c);
void *_cpu_write_brk_read_pclo(Cpu6502 *c);
void *_cpu_read_brk_read_pchi(Cpu6502 *c);
void *_cpu_read_brk_fetch(Cpu6502 *c);
void *_cpu_read_rti_read_pclo(Cpu6502 *c);
void *_cpu_read_rti_read_pchi(Cpu6502 *c);
void *_cpu_read_rti_fetch(Cpu6502 *c);
void *_cpu_read_rts_read_pchi(Cpu6502 *c);
void *_cpu_read_rts_fetch(Cpu6502 *c);
void *_cpu_read_zpg(Cpu6502 *c);
void *_cpu_write_zpg_fetch(Cpu6502 *c);
void *_cpu_write_jsr_write_pclo(Cpu6502 *c);
void *_cpu_write_jsr_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);
// void *_cpu_fetch(Cpu6502 *c);

void cpu_pulse(Cpu6502 *c) {
    tracef("cpu_pulse \n");

    c->tcu++;
    if ((c->bit_fields & PIN_READ) == PIN_READ) {
        c->data_bus = mem_read_addr(c->memmap, c->addr_bus);
    } else {
        mem_write_addr(c->memmap, c->addr_bus, c->data_bus);
    }

    u8 pd = c->data_bus;
    c->on_next_clock = (c->on_next_clock)(c);
    c->pd = pd;
}

void cpu_resb(Cpu6502 *c) {
    tracef("cpu_resb \n");
    setflag(c->p, STAT___IGNORE | STAT_B_BREAK | STAT_I_INTERRUPT);
    unsetflag(c->p, STAT_D_DECIMAL);
    // we skip the whole 2-cycle set pc part (for now anyway)
    // by hacky coincidence, not defining this sets it to $0000 which is how I set up the rom for testing
    u8 lo = mem_read_addr(c->memmap, 0xfffc);
    u8 hi = mem_read_addr(c->memmap, 0xfffd);
    c->pc = (hi << 8) | lo;
    c->addr_bus = c->pc;
    c->tcu = 0;
    setflag(c->bit_fields, PIN_READ);
    c->on_next_clock = (void *(*)(void *))(_cpu_fetch_opcode);
    logf("Reset CPU. PC set to $%04x ($fffc: $%02x, $fffd: $%02x)\n", c->pc, lo, hi);
}

void* _cpu_fetch_opcode(Cpu6502 *c) {
    c->ir = c->data_bus;
    c->addr_bus++;
    return _cpu_fetch_lo;
}

void* _cpu_fetch_lo(Cpu6502 *c) {
    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    // tracef("_cpu_fetch_lo ir: $%02X (a: %i b: %i c: %i) lo: $%02X (%i) \n",
    //                           c->ir, op_a, op_b, op_c,        lo,  lo);

    if (op_b == 1) { // zpg
        c->addr_bus = c->data_bus;
        return _cpu_read_addr;
    }

    switch (op_c)
    {
        case 0:
        {
            switch (op_b)
            {
                case 0:
                    switch (op_a)
                    {
                        case 0: // BRK
                            c->addr_bus = 0x0100 | c->sp;
                            c->data_bus = (c->pc + 2) >> 8;// pc hi
                            c->sp--;
                            setflag(c->p, STAT_I_INTERRUPT);
                            unsetflag(c->bit_fields, PIN_READ);
                            return _cpu_write_brk_write_pclo;
                        case 1: // JSR abs
                            c->addr_bus++;
                            return _cpu_fetch_hi;
                        case 2: // RTI
                            c->sp++;
                            c->addr_bus = 0x0100 | c->sp;
                            return _cpu_read_rti_read_pclo;
                        case 3: // RTS
                            c->sp++;
                            c->addr_bus = 0x0100 | c->sp;
                            return _cpu_read_rti_read_pclo;
                            break;
                        case 5: // LDY
                            c->y = c->data_bus;
                            _cpu_update_NZ_flags(c, c->y);
                            break;
                        case 6: // CPY
                            _cpu_update_NZ_flags(c, c->a - c->y);
                            setunsetflag(c->p, STAT_C_CARRY, c->a >= c->y);
                            break;
                        case 7: // CPX
                            _cpu_update_NZ_flags(c, c->a - c->x);
                            setunsetflag(c->p, STAT_C_CARRY, c->a >= c->x);
                            break;
                    }
                    c->tcu = 0;
                    c->pc+= 2;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 1: // zpg
                    c->addr_bus = c->data_bus & 0x00FF;
                    return _cpu_read_addr;
                case 2: // impl
                    switch (op_a)
                    {
                        case 0: // PHP
                            c->addr_bus = c->sp;
                            c->data_bus = c->p | STAT_B_BREAK | STAT___IGNORE;
                            unsetflag(c->bit_fields, PIN_READ);
                            return _cpu_push;
                        case 1: // PLP
                            c->addr_bus = c->sp;
                            return _cpu_read_addr;
                        case 2: // PHA
                            c->addr_bus = c->sp;
                            c->data_bus = c->a;
                            unsetflag(c->bit_fields, PIN_READ);
                            return _cpu_push;
                        case 3: // PLA
                            c->addr_bus = c->sp;
                            return _cpu_read_addr;
                        case 4: // DEY
                            c->y--;
                            break;
                        case 5: // TAY
                            c->y = c->a;
                            break;
                        case 6: // INY
                            c->y++;
                            break;
                        case 7: // INX
                            c->x++;
                            _cpu_update_NZ_flags(c, c->x);
                            break;
                    }
                    c->tcu = 0;
                    c->pc++;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 3: // abs (+ JMP ind)
                case 7: // abs,X
                    c->addr_bus++;
                    return _cpu_fetch_hi;
                case 4: // rel
                {
                    bool branch = false;
                    switch (op_a)
                    {
                        case 0: // BPL
                            branch = (c->p & STAT_N_NEGATIVE) == 0;
                            break;
                        case 1: // BMI
                            branch = (c->p & STAT_N_NEGATIVE) == STAT_N_NEGATIVE;
                            break;
                        case 2: // BVC
                            branch = (c->p & STAT_V_OVERFLOW) == 0;
                            break;
                        case 3: // BVS
                            branch = (c->p & STAT_V_OVERFLOW) == STAT_V_OVERFLOW;
                            break;
                        case 4: // BCC
                            branch = (c->p & STAT_C_CARRY) == 0;
                            break;
                        case 5: // BCS
                            branch = (c->p & STAT_C_CARRY) == STAT_C_CARRY;
                            break;
                        case 6: // BNE
                            branch = (c->p & STAT_Z_ZERO) == 0;
                            break;
                        case 7: // BEQ
                            branch = (c->p & STAT_Z_ZERO) == STAT_Z_ZERO;
                            break;
                    }
                    u16 pc_before = c->pc;
                    if (branch) {
                        c->pc += c->data_bus;
                        if ((pc_before & 0xF0) != (c->pc & 0xF0)) {
                            return _cpu_page_boundary;
                        }
                    }
                    else {
                        c->pc += 2;// next instruction
                    }
                    c->addr_bus = c->pc;
                    c->tcu = 0;
                    return _cpu_fetch_opcode;
                }
                case 5: // zpg,X
                    c->addr_bus = (c->data_bus + c->x) & 0x00FF;
                    return _cpu_read_addr;
                case 6: // impl
                    switch (op_a)
                    {
                        case 0: // CLC
                            unsetflag(c->p, STAT_C_CARRY);
                            break;
                        case 1: // SEC
                            setflag(c->p, STAT_C_CARRY);
                            break;
                        case 2: // CLI
                            unsetflag(c->p, STAT_I_INTERRUPT);
                            break;
                        case 3: // SEI
                            setflag(c->p, STAT_I_INTERRUPT);
                            break;
                        case 4: // TYA
                            c->a = c->y;
                            break;
                        case 5: // CLV
                            unsetflag(c->p, STAT_V_OVERFLOW);
                            break;
                        case 6: // CLD
                            unsetflag(c->p, STAT_D_DECIMAL);
                            break;
                        case 7: // SED
                            setflag(c->p, STAT_D_DECIMAL);
                            break;
                    }
                    c->tcu = 0;
                    c->pc++;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
            }
            break;
        }
        case 1:
        {
            switch (op_b)
            {
                case 0: // X,ind
                    break;
                case 2: // imm
                    switch (op_a)
                    {
                        case 0: // ORA
                            c->a |= c->data_bus;
                            break;
                        case 1: // AND
                            c->a &= c->data_bus;
                            break;
                        case 2: // EOR
                            c->a ^= c->data_bus;
                            break;
                        case 7: // SBC
                            c->a = ~(c->a);
                            // FALLTHROUGH!!!
                        case 3: // ADC
                        {
                            u16 a1 = (u16)(c->a) + (u16)(c->data_bus) + (u16)((c->p & STAT_C_CARRY) == STAT_C_CARRY);
                            setunsetflag(c->p, STAT_C_CARRY, a1 > 0xFF);
                            setunsetflag(c->p, STAT_V_OVERFLOW,
                                  (~((u16)(c->a) ^ (u16)(c->data_bus)))
                                & ((u16)(c->a) ^ a1)
                                & 0x0080);
                            c->a = a1 & 0xFF;
                            break;
                        }
                        case 5: // LDA
                            c->a = c->data_bus;
                            break;
                        case 6: // CMP
                            _cpu_update_NZ_flags(c, c->a - c->data_bus);
                            setunsetflag(c->p, STAT_C_CARRY, c->a >= c->data_bus);
                            break;
                        }
                    _cpu_update_NZ_flags(c, c->a);
                    c->tcu = 0;
                    c->pc += 2;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 3: // abs
                case 6: // abs,Y
                case 7: // abs,X
                    c->addr_bus++;
                    return _cpu_fetch_hi;
                case 4: // ind,Y
                    break;
                case 5: // zpg,X
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
                        c->x = c->data_bus;
                        c->tcu = 0;
                        c->pc += 2;
                        c->addr_bus = c->pc;
                        _cpu_update_NZ_flags(c, c->x);
                        return _cpu_fetch_opcode;
                    }
                    break;
                case 1: // zpg
                    c->addr_bus = c->data_bus;
                    return _cpu_read_zpg;
                case 2: // impl
                    switch (op_a)
                    {
                        case 0: // ASL
                            c->a <<= 1;
                            break;
                        case 1: // ROL
                            c->a = (c->a << 1) | (c->a >> 7);
                            break;
                        case 2: // LSR
                            c->a >>= 1;
                            break;
                        case 3: // ROR
                            c->a = (c->a >> 1) | (c->a << 7);
                            break;
                        case 4: // TXA
                            c->a = c->x;
                            break;
                        case 5: // TAX
                            c->x = c->a;
                            break;
                        case 6: // DEX
                            c->x--;
                            break;
                        case 7: // NOP
                            break;
                    }
                    c->tcu = 0;
                    c->pc++;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 3: // abs
                case 7: // abs,X
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
            }
            break;
        }
    }

    return c->on_next_clock;
}

void* _cpu_fetch_hi(Cpu6502 *c) {
    c->addr_bus = c->pd | (c->data_bus << 8);

    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    memaddr addr_no_add = c->addr_bus;
    switch (op_b)
    {
        // case 3: // abs
            // Do nothing
        case 6: // abs,Y
             c->addr_bus += c->y;
            break;
        case 7: // abs,X
            if (c->ir == 0xBE) {// LDX abs,Y
                c->addr_bus += c->y;
                break;
            }
             c->addr_bus += c->x;
            break;
    }

    if (c->ir == 0x4C) { // JMP
        c->pc = c->addr_bus;
        c->tcu = 0;
        return _cpu_fetch_opcode;
    }

    if (c->ir == 0x20) { // JSR
        c->jsr_juggle_addr_because_im_lazy = (c->data_bus << 8) | (c->pd);
        c->addr_bus = 0x0100 | c->sp;
        c->sp--;
        c->data_bus = (c->pc + 2) >> 8;
        unsetflag(c->bit_fields, PIN_READ);
        return _cpu_write_jsr_write_pclo;
    }

    if ((addr_no_add & 0xFF00) != (c->addr_bus & 0xFF00)) {
        return _cpu_page_boundray;
    }

    if (op_a == 4) { // ST_
        switch (op_c)
        {
            case 0:// STY
                c->data_bus = c->y;
                break;
            case 1:// STA
                c->data_bus = c->a;
                break;
            case 2:// STX
                c->data_bus = c->x;
                break;
            case 3:// SAX, etc
                break;
        }

        unsetflag(c->bit_fields, PIN_READ);// write
        return _cpu_write_then_fetch_add3;
    }

    return _cpu_read_addr;// Fetch ADDR value and perform operation after read
}

void *_cpu_read_addr(Cpu6502 *c) {
    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    switch (op_c)
    {
        case 0:
            switch (c->ir) {
                case 0x28: // PLP
                case 0x68: // PLA
                    c->pd = c->data_bus;
                    c->sp++;
                    return _cpu_pop;
                case 0x6C: // JMP ind
                    c->addr_bus++;
                    return _cpu_read_addr_ind;
                case 0xBC: // LDY abs,X
                    c->y = c->data_bus;
                    c->tcu = 0;
                    c->pc += 3;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
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
                    c->a |= c->data_bus;
                    _cpu_update_NZ_flags(c, c->a);
                    break;
                case 1: // AND
                    c->a &= c->data_bus;
                    _cpu_update_NZ_flags(c, c->a);
                    break;
                case 2: // EOR
                    c->a ^= c->data_bus;
                    _cpu_update_NZ_flags(c, c->a);
                    break;
                case 3: // ADC
                    break;
                case 5: // LDA
                    c->a = c->data_bus;
                    _cpu_update_NZ_flags(c, c->a);
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
                    c->x = c->data_bus;
                    _cpu_update_NZ_flags(c, c->x);
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
                case 7: // abs,X (abs, Y)
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
    c->addr_bus = (c->pd << 8) | c->data_bus;

    if (c->ir == 0x6C) { // JMP ind
        c->tcu = 0;
        c->pc = c->addr_bus;
        return _cpu_fetch_opcode;
    }

    return c->on_next_clock;
}

void *_cpu_push(Cpu6502 *c) {
    c->tcu = 0;
    c->pc++;
    c->addr_bus = c->pc;
    c->sp--;
    setflag(c->bit_fields, PIN_READ);
    return _cpu_fetch_opcode;
}

void *_cpu_pop(Cpu6502 *c) {
    switch (c->ir)
    {
        case 0x28: // PLP
            c->p = c->pd;
            unsetflag(c->p, STAT_B_BREAK);
            unsetflag(c->p, STAT___IGNORE);
            break;
    }
    c->pc++;
    c->tcu = 0;
    c->addr_bus = c->pc;
    return _cpu_fetch_opcode;
}

void *_cpu_page_boundray(Cpu6502 *c) {
    c = c;// ignore warning
    return _cpu_read_addr;
}

void *_cpu_write_then_fetch_add3(Cpu6502 *c) {
    c->pc += 3;
    c->tcu = 0;
    c->addr_bus = c->pc;
    setflag(c->bit_fields, PIN_READ);// read
    return _cpu_fetch_opcode;
}

void *_cpu_write_brk_write_pclo(Cpu6502 *c) {
    c->addr_bus = 0x0100 | c->sp;
    c->data_bus = 0xFF & (c->pc + 2);
    c->sp--;
    return _cpu_write_brk_write_sr;
}

void *_cpu_write_brk_write_sr(Cpu6502 *c) {
    c->addr_bus = 0x0100 | c->sp;
    c->data_bus = c->p & STAT_B_BREAK;
    c->sp--;
    return _cpu_write_brk_read_pclo;
}

void *_cpu_write_brk_read_pclo(Cpu6502 *c) {
    c->addr_bus = 0xFFFE;
    setflag(c->bit_fields, PIN_READ);
    return _cpu_write_brk_read_pclo;
}

void *_cpu_read_brk_read_pchi(Cpu6502 *c) {
    c->addr_bus = 0xFFFE;
    return _cpu_read_brk_fetch;
}

void *_cpu_read_brk_fetch(Cpu6502 *c) {
    c->pc = (c->data_bus << 8) | c->pd;
    c->addr_bus = c->pc;
    c->tcu = 0;
    return _cpu_fetch_opcode;
}

void *_cpu_read_rti_read_pclo(Cpu6502 *c) {;
    c->sp++;
    c->addr_bus = 0x0100 | c->sp;
    return _cpu_read_rti_read_pchi;
}

void *_cpu_read_rti_read_pchi(Cpu6502 *c) {
    c->pc = (c->data_bus << 8) | c->pd;
    c->sp++;
    c->addr_bus = 0x0100 | c->sp;
    return _cpu_read_rti_fetch;
}

void *_cpu_read_rti_fetch(Cpu6502 *c) {
    c->p = c->data_bus;
    c->addr_bus = c->pc;
    c->tcu = 0;
    return _cpu_fetch_opcode;
}

void *_cpu_read_rts_read_pchi(Cpu6502 *c) {
    c->sp++;
    c->addr_bus = 0x0100 | c->sp;
    return _cpu_read_rts_fetch;
}

void *_cpu_read_rts_fetch(Cpu6502 *c) {
    c->pc = (c->data_bus << 8) | c->pd;
    c->addr_bus = c->pc;
    c->tcu = 0;
    return _cpu_fetch_opcode;
}

void *_cpu_read_zpg(Cpu6502 *c) {
    int op_c = (c->ir & 0b00000011) >> 0;

    switch (op_c)
    {
        case 0: // ASL
        {
            u8 carry = c->data_bus >> 7 & 0x01;
            c->data_bus = c->data_bus << 1;
            setunsetflag(c->p, STAT_C_CARRY, carry);
            _cpu_update_NZ_flags(c, c->data_bus);
            break;
        }
        case 1: // ROL
        {
            u8 carry = c->data_bus >> 7 & 0x01;
            c->data_bus = (c->data_bus << 1) | carry;
            setunsetflag(c->p, STAT_C_CARRY, carry);
            _cpu_update_NZ_flags(c, c->data_bus);
            break;
        }
        case 2: // LSR
        {
            u8 carry = c->data_bus & 0x01;
            c->data_bus = c->data_bus >> 1;
            setunsetflag(c->p, STAT_C_CARRY, carry);
            _cpu_update_NZ_flags(c, c->data_bus);
            break;
        }
        case 3: // ROR
        {
            u8 carry = c->data_bus & 0x01;
            c->data_bus = (c->data_bus >> 1) | (carry << 7);
            setunsetflag(c->p, STAT_C_CARRY, carry);
            _cpu_update_NZ_flags(c, c->data_bus);
            break;
        }
        case 4: // STX
            c->data_bus = c->x;
            break;
        case 5: // LDX
            c->x = c->data_bus;
            _cpu_update_NZ_flags(c, c->x);
            return _cpu_write_zpg_fetch(c);// cuz I'm lazy
        case 6: // DEC
            c->data_bus--;
            _cpu_update_NZ_flags(c, c->x);
            break;
        case 7: // INC
            c->data_bus++;
            _cpu_update_NZ_flags(c, c->x);
            break;
    }

    // data_bus = data_bus;

    return _cpu_write_zpg_fetch;
}

void *_cpu_write_zpg_fetch(Cpu6502 *c) {
    c->pc += 2;
    c->tcu = 0;
    c->addr_bus = c->pc;
    unsetflag(c->bit_fields, PIN_READ);
    return _cpu_fetch_opcode;
}

void *_cpu_write_jsr_write_pclo(Cpu6502 *c) {
    c->addr_bus = 0x0100 | c->sp;
    c->sp--;
    c->data_bus = (c->pc + 2) & 0xFF;
    return _cpu_write_jsr_fetch;
}

void *_cpu_write_jsr_fetch(Cpu6502 *c) {
    c->pc = c->jsr_juggle_addr_because_im_lazy;
    c->addr_bus = c->pc;
    c->tcu = 0;
    setflag(c->bit_fields, PIN_READ);
    return _cpu_fetch_opcode;
}
