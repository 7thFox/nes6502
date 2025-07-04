#include "headers/cpu6502.h"

void _cpu_update_NZ_flags(Cpu6502 *c, u8 val) {
    setunsetflag(c->p, STAT_N_NEGATIVE, val & 0x80);
    setunsetflag(c->p, STAT_Z_ZERO, val == 0x00);
}

void compare(Cpu6502 *c, u8 reg) {
    _cpu_update_NZ_flags(c, reg - c->data_bus);
    setunsetflag(c->p, STAT_C_CARRY, reg >= c->data_bus);
}

void *_cpu_fetch_opcode(Cpu6502 *c);
void *_cpu_fetch_opcode_add1(Cpu6502 *c);
void *_cpu_fetch_opcode_add2(Cpu6502 *c);
void *_cpu_fetch_lo(Cpu6502 *c);
void *_cpu_fetch_hi(Cpu6502 *c);
void *_cpu_read_addr(Cpu6502 *c);
void *_cpu_page_boundary(Cpu6502 *c);
void *_cpu_read_addr_ind(Cpu6502 *c);
void *_cpu_push(Cpu6502 *c);
void *_cpu_pop(Cpu6502 *c);
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
void *_cpu_write_jsr_missing_extra_cycle(Cpu6502 *c);
void *_cpu_write_jsr_fetch(Cpu6502 *c);
void *_cpu_read_ind_read_addrhi(Cpu6502 *c);
void *_cpu_read_ind_read_val(Cpu6502 *c);
void *_cpu_rel_addr_inc(Cpu6502 *c);

void cpu_pulse(Cpu6502 *c) {
    tracef("cpu_pulse \n");

    c->cyc++;
    c->tcu++;
    if ((c->bit_fields & PIN_READ) == PIN_READ) {
        c->data_bus = mem_read_addr(c->memmap, c->addr_bus);
    }
    else {
        mem_write_addr(c->memmap, c->addr_bus, c->data_bus);
    }

    u8 pd            = c->data_bus;
    c->on_next_clock = (c->on_next_clock)(c);
    c->pd            = pd;
}

void cpu_resb(Cpu6502 *c) {
    tracef("cpu_resb \n");
    setflag(c->p, STAT___IGNORE | STAT_I_INTERRUPT);
    unsetflag(c->p, STAT_D_DECIMAL);

    // we skip the whole 2-cycle set pc part (for now anyway)
    // by hacky coincidence, not defining this sets it to $0000 which is how I set up the rom for testing
    u8 lo       = mem_read_addr(c->memmap, 0xfffc);
    u8 hi       = mem_read_addr(c->memmap, 0xfffd);
    c->pc       = (hi << 8) | lo;
    c->addr_bus = c->pc;
    c->tcu      = 0;
    c->cyc      = 0;
    setflag(c->bit_fields, PIN_READ);
    c->on_next_clock = (void *(*)(void *))(_cpu_fetch_opcode);
    infof("Reset CPU. PC set to $%04x ($fffc: $%02x, $fffd: $%02x)\n", c->pc, lo, hi);
}

void *_cpu_fetch_opcode(Cpu6502 *c) {
    c->ir = c->data_bus;
    c->addr_bus++;
    return _cpu_fetch_lo;
}

void *_cpu_fetch_opcode_add1(Cpu6502 *c) {
    c->addr_bus = c->pc;
    c->tcu      = 0;
    setflag(c->bit_fields, PIN_READ);
    return _cpu_fetch_opcode;
}
void *_cpu_fetch_opcode_add2(Cpu6502 *c __attribute__((unused))) { return _cpu_fetch_opcode_add1; }

void *_cpu_fetch_lo(Cpu6502 *c) {
    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    // tracef("_cpu_fetch_lo ir: $%02X (a: %i b: %i c: %i) lo: $%02X (%i) \n",
    //                           c->ir, op_a, op_b, op_c,        lo,  lo);

    if (op_b == 1) { // zpg
        if (op_a == 4) { // ST_ ops
            c->addr_bus = c->data_bus & 0x00FF;
            switch (c->ir) {
                case 0x84: // STY
                    c->data_bus = c->y;
                    break;
                case 0x85: // STA
                    c->data_bus = c->a;
                    break;
                case 0x86: // STX
                    c->data_bus = c->x;
                    break;
            }
            unsetflag(c->bit_fields, PIN_READ);
            return _cpu_write_zpg_fetch;
        }
        else {
            c->addr_bus = c->data_bus  & 0x00FF;
            return _cpu_read_addr;
        }
    }

    switch (op_c) {
        case 0:
        {
            switch (op_b) {
                case 0:
                    switch (op_a) {
                        case 0: // BRK
                            c->addr_bus = 0x0100 | c->sp;
                            c->data_bus = (c->pc + 2) >> 8; // pc hi
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
                            return _cpu_read_rts_read_pchi;
                        case 5: // LDY
                            c->y = c->data_bus;
                            _cpu_update_NZ_flags(c, c->y);
                            break;
                        case 6: // CPY
                            compare(c, c->y);
                            break;
                        case 7: // CPX
                            compare(c, c->x);
                            break;
                    }
                    c->tcu = 0;
                    c->pc += 2;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
                case 1: // zpg
                    c->addr_bus = c->data_bus & 0x00FF;
                    if (c->ir == 0x84) { // STY
                        c->data_bus = c->y;
                        return _cpu_write_zpg_fetch;
                    }
                    return _cpu_read_addr;
                case 2: // impl
                    switch (op_a) {
                        case 0: // PHP
                            c->addr_bus = c->sp + 0x100;
                            c->data_bus = c->p | STAT_B_BREAK | STAT___IGNORE;
                            unsetflag(c->bit_fields, PIN_READ);
                            return _cpu_push;
                        case 1: // PLP
                            c->sp++;
                            c->addr_bus = c->sp + 0x100;
                            return _cpu_read_addr;
                        case 2: // PHA
                            c->addr_bus = c->sp + 0x100;
                            c->data_bus = c->a;
                            unsetflag(c->bit_fields, PIN_READ);
                            return _cpu_push;
                        case 3: // PLA
                            c->sp++;
                            c->addr_bus = c->sp + 0x100;
                            return _cpu_read_addr;
                        case 4: // DEY
                            c->y--;
                            _cpu_update_NZ_flags(c, c->y);
                            break;
                        case 5: // TAY
                            c->y = c->a;
                            _cpu_update_NZ_flags(c, c->y);
                            break;
                        case 6: // INY
                            c->y++;
                            _cpu_update_NZ_flags(c, c->y);
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
                    switch (op_a) {
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
                    c->pc += 2; // next instruction
                    if (branch) {
                        return _cpu_rel_addr_inc;
                    }
                    c->addr_bus = c->pc;
                    c->tcu      = 0;
                    return _cpu_fetch_opcode;
                }
                case 5: // zpg,X
                    c->addr_bus = (c->data_bus + c->x) & 0x00FF;
                    return _cpu_read_addr;
                case 6: // impl
                    switch (op_a) {
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
                            _cpu_update_NZ_flags(c, c->a);
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
            switch (op_b) {
                case 0: // X,ind
                    c->addr_bus = (c->data_bus + c->x) & 0x00FF; // no carry
                    return _cpu_read_ind_read_addrhi;
                case 2: // imm
                    switch (op_a) {
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
                            c->data_bus = ~c->data_bus;
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
                            compare(c, c->a);
                            // need to skip setNZ below
                            c->tcu = 0;
                            c->pc += 2;
                            c->addr_bus = c->pc;
                            return _cpu_fetch_opcode;
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
                    c->addr_bus = c->data_bus;
                    return _cpu_read_ind_read_addrhi;
                case 5: // zpg,X
                    break;
            }
            break;
        }
        case 2:
        {
            switch (op_b) {
                case 0: // imm
                    if (op_a == 5) { // LDX
                        c->x   = c->data_bus;
                        c->tcu = 0;
                        c->pc += 2;
                        c->addr_bus = c->pc;
                        _cpu_update_NZ_flags(c, c->x);
                        return _cpu_fetch_opcode;
                    }
                    break;
                case 1: // STX zpg
                    c->addr_bus = c->data_bus & 0x00FF;
                    c->data_bus = c->x;
                    unsetflag(c->bit_fields, PIN_READ);
                    return _cpu_write_zpg_fetch;
                case 2: // impl
                    switch (op_a) {
                        case 0: // ASL
                            setunsetflag(c->p, STAT_C_CARRY, (c->a & 0x80) == 0x80);
                            c->a <<= 1;
                            _cpu_update_NZ_flags(c, c->a);
                            break;
                        case 1: // ROL
                        {
                            u8 c0 = c->p & STAT_C_CARRY;
                            setunsetflag(c->p, STAT_C_CARRY, (c->a & 0x80) == 0x80);
                            c->a = c->a << 1 | c0;
                            _cpu_update_NZ_flags(c, c->a);
                            break;
                        }
                        case 2: // LSR
                            setunsetflag(c->p, STAT_C_CARRY, c->a & 0x01);
                            c->a >>= 1;
                            _cpu_update_NZ_flags(c, c->a);
                            break;
                        case 3: // ROR
                        {
                            u8 c0 = c->p & STAT_C_CARRY;
                            c->p = (c->p & ~STAT_C_CARRY) | (c->a & STAT_C_CARRY);
                            c->a = c->a >> 1 | c0 << 7;
                            _cpu_update_NZ_flags(c, c->a);
                            break;
                        }
                            // setunsetflag(c->p, STAT_C_CARRY, c->a & 0x01);
                            // c->a >>=1;
                            // if ((c->p & STAT_C_CARRY) == STAT_C_CARRY) {
                            //     c->a |= 0x80;
                            // }
                            // _cpu_update_NZ_flags(c, c->a);
                            break;
                        case 4: // TXA
                            c->a = c->x;
                            _cpu_update_NZ_flags(c, c->a);
                            break;
                        case 5: // TAX
                            c->x = c->a;
                            _cpu_update_NZ_flags(c, c->x);
                            break;
                        case 6: // DEX
                            c->x--;
                            _cpu_update_NZ_flags(c, c->x);
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
                    switch (op_a) {
                        case 4: // TXS
                            c->sp = c->x;
                            break;
                        case 5: // TSX
                            c->x = c->sp + 0x100;
                            _cpu_update_NZ_flags(c, c->x);
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

void *_cpu_fetch_hi(Cpu6502 *c) {
    c->addr_bus = c->pd | (c->data_bus << 8);

    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    memaddr addr_no_add = c->addr_bus;
    switch (op_b) {
        // case 3: // abs
            // Do nothing
        case 6: // abs,Y
            c->addr_bus += c->y;
            break;
        case 7: // abs,X
            if (c->ir == 0xBE) { // LDX abs,Y
                c->addr_bus += c->y;
                break;
            }
            c->addr_bus += c->x;
            break;
    }

    if (c->ir == 0x4C) { // JMP
        c->pc  = c->addr_bus;
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
        return _cpu_page_boundary;
    }

    if (op_a == 4) { // ST_
        switch (op_c) {
            case 0: // STY
                c->data_bus = c->y;
                break;
            case 1: // STA
                c->data_bus = c->a;
                break;
            case 2: // STX
                c->data_bus = c->x;
                break;
            case 3: // SAX, etc
                break;
        }

        unsetflag(c->bit_fields, PIN_READ); // write
        return _cpu_write_then_fetch_add3;
    }

    return _cpu_read_addr; // Fetch ADDR value and perform operation after read
}

void *_cpu_read_addr(Cpu6502 *c) {
    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;
    int op_c = (c->ir & 0b00000011) >> 0;

    switch (op_c) {
        case 0:
            if (op_b == 1 || op_b == 3) {
                switch (op_a) {
                    case 1: // BIT
                    {
                        u8 bits = STAT_N_NEGATIVE | STAT_V_OVERFLOW;
                        c->p = (c->p & ~bits) | (c->data_bus & bits);
                        setunsetflag(c->p, STAT_Z_ZERO, ((c->data_bus & c->a) == 0));
                        break;
                    }
                    case 5: // LDY
                        c->y = c->data_bus;
                        _cpu_update_NZ_flags(c, c->y);
                        break;
                    case 6: // CPY
                        compare(c, c->y);
                        break;
                    case 7: // CPX
                        compare(c, c->x);
                        break;
                }
                c->tcu = 0;
                c->pc += 2;
                c->addr_bus = c->pc;
                return _cpu_fetch_opcode;
            }
            switch (c->ir) {
                case 0x28: // PLP
                case 0x68: // PLA
                    c->pd = c->data_bus;
                    return _cpu_pop;
                case 0x6C: // JMP ind
                    c->addr_bus++;
                    return _cpu_read_addr_ind;
                case 0xBC: // LDY abs,X
                    c->y   = c->data_bus;
                    c->tcu = 0;
                    c->pc += 3;
                    c->addr_bus = c->pc;
                    return _cpu_fetch_opcode;
            }
            break;
        case 1:
            // X,ind and ind,Y have already been decoded by this point
            switch (op_a) {
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
                    compare(c, c->a);
                    break;
                case 7: // SBC
                    break;
            }
            switch (op_b) {
                // case 2: imm
                case 0: // X,ind
                case 1: // zpg
                case 4: // ind,Y
                case 5: // zpg,X
                    c->pc += 2;
                    break;
                case 3: // abs
                case 6: // abs,Y
                case 7: // abs,X
                    c->pc += 3;
                    break;
            }
            c->tcu      = 0;
            c->addr_bus = c->pc;
            return _cpu_fetch_opcode;
        case 2:
            switch (op_a) {
                case 0: // ASL
                    break;
                case 1: // ROL
                    break;
                case 2: // LSR
                    break;
                case 3: // ROR
                    break;
                case 4: // STX
                    c->data_bus = c->x;
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
            switch (op_b) {
                case 1: // zpg
                case 5: // zpg,X
                    c->pc += 2;
                    break;
                case 3: // abs
                case 7: // abs,X (abs, Y)
                    c->pc += 3;
                    break;
            }
            c->tcu      = 0;
            c->addr_bus = c->pc;
            return _cpu_fetch_opcode;
    }
    return c->on_next_clock;
}

void *_cpu_page_boundary(Cpu6502 *c) {
    c->tcu      = 0;
    c->addr_bus = c->pc;
    return _cpu_fetch_opcode;
}

void *_cpu_read_addr_ind(Cpu6502 *c) {
    c->addr_bus = (c->pd << 8) | c->data_bus;

    if (c->ir == 0x6C) { // JMP ind
        c->tcu = 0;
        c->pc  = c->addr_bus;
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
    switch (c->ir) {
        case 0x28: // PLP
        {
            u8 keep = c->p & (STAT_B_BREAK | STAT___IGNORE);
            c->p = c->pd & ~STAT___IGNORE & ~STAT___IGNORE;
            c->p |= keep;
            break;
        }
        case 0x68: // PLA
        {
            c->a = c->pd;
            _cpu_update_NZ_flags(c, c->a);
            break;
        }
    }
    c->pc++;
    c->tcu      = 0;
    c->addr_bus = c->pc;
    return _cpu_fetch_opcode;
}

void *_cpu_write_then_fetch_add3(Cpu6502 *c) {
    c->pc += 3;
    c->tcu      = 0;
    c->addr_bus = c->pc;
    setflag(c->bit_fields, PIN_READ); // read
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
    return _cpu_read_brk_read_pchi;
}

void *_cpu_read_brk_read_pchi(Cpu6502 *c) {
    c->addr_bus = 0xFFFF;
    return _cpu_read_brk_fetch;
}

void *_cpu_read_brk_fetch(Cpu6502 *c) {
    c->pc       = (c->data_bus << 8) | c->pd;
    c->addr_bus = c->pc;
    c->tcu      = 0;
    return _cpu_fetch_opcode;
}

void *_cpu_read_rti_read_pclo(Cpu6502 *c) {
    c->p = c->data_bus & (~STAT_B_BREAK);
    c->sp++;
    c->addr_bus = 0x0100 | c->sp;
    return _cpu_read_rti_read_pchi;
}

void *_cpu_read_rti_read_pchi(Cpu6502 *c) {
    c->sp++;
    c->addr_bus = 0x0100 | c->sp;
    return _cpu_read_rti_fetch;
}

void *_cpu_read_rti_fetch(Cpu6502 *c) {
    c->pc       = (c->data_bus << 8) | c->pd;
    c->addr_bus = c->pc;
    c->tcu      = 0;
    return _cpu_fetch_opcode;
}

void *_cpu_read_rts_read_pchi(Cpu6502 *c) {
    c->sp++;
    c->addr_bus = 0x0100 | c->sp;
    return _cpu_read_rts_fetch;
}

void *_cpu_read_rts_fetch(Cpu6502 *c) {
    c->pc = (c->data_bus << 8) | c->pd;
    c->pc++; // RTS adds 1 to the pushed pc
    return _cpu_fetch_opcode_add2;
}

void *_cpu_read_zpg(Cpu6502 *c) {
    int op_c = (c->ir & 0b00000011) >> 0;

    switch (op_c) {
        case 0: // ASL
        {
            u8 carry    = c->data_bus >> 7 == 0x01;
            c->data_bus = c->data_bus << 1;
            setunsetflag(c->p, STAT_C_CARRY, carry);
            _cpu_update_NZ_flags(c, c->data_bus);
            break;
        }
        case 1: // ROL
        {
            u8 carry    = c->data_bus >> 7 & 0x01;
            c->data_bus = (c->data_bus << 1) | carry;
            setunsetflag(c->p, STAT_C_CARRY, carry);
            _cpu_update_NZ_flags(c, c->data_bus);
            break;
        }
        case 2: // LSR
        {
            u8 carry    = c->data_bus & 0x01;
            c->data_bus = c->data_bus >> 1;
            setunsetflag(c->p, STAT_C_CARRY, carry);
            _cpu_update_NZ_flags(c, c->data_bus);
            break;
        }
        case 3: // ROR
        {
            u8 carry    = c->data_bus & 0x01;
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
            return _cpu_write_zpg_fetch(c); // cuz I'm lazy
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
    c->tcu      = 0;
    c->addr_bus = c->pc;
    setflag(c->bit_fields, PIN_READ);
    return _cpu_fetch_opcode;
}

void *_cpu_write_jsr_write_pclo(Cpu6502 *c) {
    c->addr_bus = 0x0100 | c->sp;
    c->sp--;
    c->data_bus = (c->pc + 2) & 0xFF;
    return _cpu_write_jsr_missing_extra_cycle;
}

void *_cpu_write_jsr_fetch(Cpu6502 *c) {
    c->pc       = c->jsr_juggle_addr_because_im_lazy;
    c->addr_bus = c->pc;
    c->tcu      = 0;
    setflag(c->bit_fields, PIN_READ);
    return _cpu_fetch_opcode;
}

void *_cpu_write_jsr_missing_extra_cycle(Cpu6502 *c __attribute__((unused))) {
    return _cpu_write_jsr_fetch;
}

void *_cpu_read_ind_read_addrhi(Cpu6502 *c) {
    int op_b = (c->ir & 0b00011100) >> 2;
    if (op_b == 0) { // X,ind
        c->addr_bus = (c->pd + c->x + 1) & 0x00FF; // without carry
    }
    else if (op_b == 4) { // ind,Y
        c->addr_bus = (c->pd + 1) & 0xFF;
    }
    return _cpu_read_ind_read_val;
}

void *_cpu_read_ind_read_val(Cpu6502 *c) {
    int op_a = (c->ir & 0b11100000) >> 5;
    int op_b = (c->ir & 0b00011100) >> 2;

    if (op_b == 0) { // X,ind
        c->addr_bus = (c->data_bus << 8) | c->pd;
    }
    else if (op_b == 4) { // ind,Y
        c->addr_bus = (c->data_bus + c->y);
        u16 before  = c->addr_bus;
        c->addr_bus += c->y;
        if ((c->addr_bus & 0xFF00) != (before & 0xFF00)) {
            return _cpu_page_boundary;
        }
    }

    if (op_a == 4) { // STA
        c->data_bus = c->a;
        unsetflag(c->bit_fields, PIN_READ);
        return _cpu_write_zpg_fetch;
    }

    return _cpu_read_addr;
}


void *_cpu_rel_addr_inc(Cpu6502 *c) {
    u16 pc_before = c->pc;
    c->pc += c->data_bus;

    if ((pc_before & 0xFF00) != (c->pc & 0xFF00)) {
        return _cpu_page_boundary;
    }

    return _cpu_page_boundary(c);
}