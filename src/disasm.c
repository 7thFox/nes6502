#include "headers/disasm.h"

typedef enum {
    AM_A,    // Accumulator
    AM_abs,  // absolute
    AM_absX, // absolute, X-indexed
    AM_absY, // absolute, Y-indexed
    AM_imm,  // immediate
    AM_impl, // implied
    AM_ind,  // indirect
    AM_Xind, // X-indexed, indirect
    AM_indY, // indirect, Y-indexed
    AM_rel,  // relative
    AM_zpg,  // zeropage
    AM_zpgX, // zeropage, X-indexed
    AM_zpgY, // zeropage, Y-indexed
} AddressingMode;

typedef struct {
    const char *mnemonic;
    AddressingMode addressing_mode;
} InstructionMeta;

int inst_get_size(InstructionMeta i) {
    switch (i.addressing_mode)
    {
        case AM_A:
        case AM_impl:
            return 1;
        case AM_zpg:
        case AM_imm:
        case AM_Xind:
        case AM_indY:
        case AM_rel:
        case AM_zpgX:
        case AM_zpgY:
            return 2;
        case AM_abs:
        case AM_absX:
        case AM_absY:
        case AM_ind:
            return 3;
    }
    return 0;
}

#define m(m, addr, _) { .mnemonic = m, .addressing_mode = addr}

InstructionMeta INSTRUCTIONS[0x100] = {
    m("BRK", AM_impl, op_brk), m("ORA", AM_Xind, op_ora), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("ORA", AM_zpg, op_ora),  m("ASL", AM_zpg, op_asl),  m("???", AM_impl, op____), m("PHP", AM_impl, op_php), m("ORA", AM_imm, op_ora),  m("ASL", AM_A, op_asl),    m("???", AM_impl, op____), m("???", AM_impl, op____), m("ORA", AM_abs, op_ora),  m("ASL", AM_abs, op_asl),  m("???", AM_impl, op____),
    m("BPL", AM_rel, op_bpl),  m("ORA", AM_indY, op_ora), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("ORA", AM_zpgX, op_ora), m("ASL", AM_zpgX, op_asl), m("???", AM_impl, op____), m("CLC", AM_impl, op_clc), m("ORA", AM_absY, op_ora), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("ORA", AM_absX, op_ora), m("ASL", AM_absX, op_asl), m("???", AM_impl, op____),
    m("JSR", AM_abs, op_jsr),  m("AND", AM_Xind, op_and), m("???", AM_impl, op____), m("???", AM_impl, op____), m("BIT", AM_zpg, op_bit),  m("AND", AM_zpg, op_and),  m("ROL", AM_zpg, op_rol),  m("???", AM_impl, op____), m("PLP", AM_impl, op_plp), m("AND", AM_imm, op_and),  m("ROL", AM_A, op_rol),    m("???", AM_impl, op____), m("BIT", AM_abs, op_bit),  m("AND", AM_abs, op_and),  m("ROL", AM_abs, op_rol),  m("???", AM_impl, op____),
    m("BMI", AM_rel, op_bmi),  m("AND", AM_indY, op_and), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("AND", AM_zpgX, op_and), m("ROL", AM_zpgX, op_rol), m("???", AM_impl, op____), m("SEC", AM_impl, op_sec), m("AND", AM_absY, op_and), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("AND", AM_absX, op_and), m("ROL", AM_absX, op_rol), m("???", AM_impl, op____),
    m("RTI", AM_impl, op_rti), m("EOR", AM_Xind, op_eor), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("EOR", AM_zpg, op_eor),  m("LSR", AM_zpg, op_lsr),  m("???", AM_impl, op____), m("PHA", AM_impl, op_pha), m("EOR", AM_imm, op_eor),  m("LSR", AM_A, op_lsr),    m("???", AM_impl, op____), m("JMP", AM_abs, op_jmp),  m("EOR", AM_abs, op_eor),  m("LSR", AM_abs, op_lsr),  m("???", AM_impl, op____),
    m("BVC", AM_rel, op_bvc),  m("EOR", AM_indY, op_eor), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("EOR", AM_zpgX, op_eor), m("LSR", AM_zpgX, op_lsr), m("???", AM_impl, op____), m("CLI", AM_impl, op_cli), m("EOR", AM_absY, op_eor), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("EOR", AM_absX, op_eor), m("LSR", AM_absX, op_lsr), m("???", AM_impl, op____),
    m("RTS", AM_impl, op_rts), m("ADC", AM_Xind, op_adc), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("ADC", AM_zpg, op_adc),  m("ROR", AM_zpg, op_ror),  m("???", AM_impl, op____), m("PLA", AM_impl, op_pla), m("ADC", AM_imm, op_adc),  m("ROR", AM_A, op_ror),    m("???", AM_impl, op____), m("JMP", AM_ind, op_jmp),  m("ADC", AM_abs, op_adc),  m("ROR", AM_abs, op_ror),  m("???", AM_impl, op____),
    m("BVS", AM_rel, op_bvs),  m("ADC", AM_indY, op_adc), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("ADC", AM_zpgX, op_adc), m("ROR", AM_zpgX, op_ror), m("???", AM_impl, op____), m("SEI", AM_impl, op_sei), m("ADC", AM_absY, op_adc), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("ADC", AM_absX, op_adc), m("ROR", AM_absX, op_ror), m("???", AM_impl, op____),
    m("???", AM_impl, op____), m("STA", AM_Xind, op_sta), m("???", AM_impl, op____), m("???", AM_impl, op____), m("STY", AM_zpg, op_sty),  m("STA", AM_zpg, op_sta),  m("STX", AM_zpg, op_stx),  m("???", AM_impl, op____), m("DEY", AM_impl, op_dey), m("???", AM_impl, op____), m("TXA", AM_impl, op_txa), m("???", AM_impl, op____), m("STY", AM_abs, op_sty),  m("STA", AM_abs, op_sta),  m("STX", AM_abs, op_stx),  m("???", AM_impl, op____),
    m("BCC", AM_rel, op_bcc),  m("STA", AM_indY, op_sta), m("???", AM_impl, op____), m("???", AM_impl, op____), m("STY", AM_zpgX, op_sty), m("STA", AM_zpgX, op_sta), m("STX", AM_zpgY, op_stx), m("???", AM_impl, op____), m("TYA", AM_impl, op_tya), m("STA", AM_absY, op_sta), m("TXS", AM_impl, op_txs), m("???", AM_impl, op____), m("???", AM_impl, op____), m("STA", AM_absX, op_sta), m("???", AM_impl, op____), m("???", AM_impl, op____),
    m("LDY", AM_imm, op_ldy),  m("LDA", AM_Xind, op_lda), m("LDX", AM_imm, op_ldx),  m("???", AM_impl, op____), m("LDY", AM_zpg, op_ldy),  m("LDA", AM_zpg, op_lda),  m("LDX", AM_zpg, op_ldx),  m("???", AM_impl, op____), m("TAY", AM_impl, op_tay), m("LDA", AM_imm, op_lda),  m("TAX", AM_impl, op_tax), m("???", AM_impl, op____), m("LDY", AM_abs, op_ldy),  m("LDA", AM_abs, op_lda),  m("LDX", AM_abs, op_ldx),  m("???", AM_impl, op____),
    m("BCS", AM_rel, op_bcs),  m("LDA", AM_indY, op_lda), m("???", AM_impl, op____), m("???", AM_impl, op____), m("LDY", AM_zpgX, op_ldy), m("LDA", AM_zpgX, op_lda), m("LDX", AM_zpgY, op_ldx), m("???", AM_impl, op____), m("CLV", AM_impl, op_clv), m("LDA", AM_absY, op_lda), m("TSX", AM_impl, op_tsx), m("???", AM_impl, op____), m("LDY", AM_absX, op_ldy), m("LDA", AM_absX, op_lda), m("LDX", AM_absY, op_ldx), m("???", AM_impl, op____),
    m("CPY", AM_imm, op_cpy),  m("CMP", AM_Xind, op_cmp), m("???", AM_impl, op____), m("???", AM_impl, op____), m("CPY", AM_zpg, op_cpy),  m("CMP", AM_zpg, op_cmp),  m("DEC", AM_zpg, op_dec),  m("???", AM_impl, op____), m("INY", AM_impl, op_iny), m("CMP", AM_imm, op_cmp),  m("DEX", AM_impl, op_dex), m("???", AM_impl, op____), m("CPY", AM_abs, op_cpy),  m("CMP", AM_abs, op_cmp),  m("DEC", AM_abs, op_dec),  m("???", AM_impl, op____),
    m("BNE", AM_rel, op_bne),  m("CMP", AM_indY, op_cmp), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("CMP", AM_zpgX, op_cmp), m("DEC", AM_zpgX, op_dec), m("???", AM_impl, op____), m("CLD", AM_impl, op_cld), m("CMP", AM_absY, op_cmp), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("CMP", AM_absX, op_cmp), m("DEC", AM_absX, op_dec), m("???", AM_impl, op____),
    m("CPX", AM_imm, op_cpx),  m("SBC", AM_Xind, op_sbc), m("???", AM_impl, op____), m("???", AM_impl, op____), m("CPX", AM_zpg, op_cpx),  m("SBC", AM_zpg, op_sbc),  m("INC", AM_zpg, op_inc),  m("???", AM_impl, op____), m("INX", AM_impl, op_inx), m("SBC", AM_imm, op_sbc),  m("NOP", AM_impl, op_nop), m("???", AM_impl, op____), m("CPX", AM_abs, op_cpx),  m("SBC", AM_abs, op_sbc),  m("INC", AM_abs, op_inc),  m("???", AM_impl, op____),
    m("BEQ", AM_rel, op_beq),  m("SBC", AM_indY, op_sbc), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("SBC", AM_zpgX, op_sbc), m("INC", AM_zpgX, op_inc), m("???", AM_impl, op____), m("SED", AM_impl, op_sed), m("SBC", AM_absY, op_sbc), m("???", AM_impl, op____), m("???", AM_impl, op____), m("???", AM_impl, op____), m("SBC", AM_absX, op_sbc), m("INC", AM_absX, op_inc), m("???", AM_impl, op____),

};

Disassembler *create_disassembler() {
    Disassembler *d = malloc(sizeof(Disassembler));
    memset(d->_disasm_text, 0, N_MAX_DISASM * N_MAX_TEXT_SIZE);
    // memset(d->_disasm_bytes, 0, N_MAX_DISASM * N_MAX_BYTE_SIZE);
    return d;
}

Disassembly disasm(Disassembler* d, u8 *data_aligned, size_t data_size, int n) {
    if (n > N_MAX_DISASM) n = N_MAX_DISASM;
    u16 iData = 0;
    u16 iInst = 0;
    while (iInst < n && iData < data_size) {
        u8 opcode = data_aligned[iData];
        u8 lo = iData + 1u < data_size ? data_aligned[iData+1] : 0;
        u8 hi = iData + 2u < data_size ? data_aligned[iData+2] : 0;
        u16 param16 = (hi << 8) | lo;
        InstructionMeta inst = INSTRUCTIONS[opcode];

        d->_disasm_offsets[iInst] = iData;

        switch (inst.addressing_mode)
        {
            case AM_A:
                sprintf(d->_disasm_text[iInst], "%s A", inst.mnemonic);
                break;
            case AM_abs:
                sprintf(d->_disasm_text[iInst], "%s $%04x (%d)", inst.mnemonic, param16, param16);
                break;
            case AM_absX:
                sprintf(d->_disasm_text[iInst], "%s $%04x,X (%d)", inst.mnemonic, param16, param16);
                break;
            case AM_absY:
                sprintf(d->_disasm_text[iInst], "%s $%04x,Y (%d)", inst.mnemonic, param16, param16);
                break;
            case AM_imm:
                sprintf(d->_disasm_text[iInst], "%s #$%02x (%d)", inst.mnemonic, lo, lo);
                break;
            case AM_impl:
                sprintf(d->_disasm_text[iInst], "%s", inst.mnemonic);
                break;
            case AM_ind:
                sprintf(d->_disasm_text[iInst], "%s ($%04x) (%d)", inst.mnemonic, param16, param16);
                break;
            case AM_Xind:
                sprintf(d->_disasm_text[iInst], "%s ($%02x,X) (%d)", inst.mnemonic, lo, lo);
                break;
            case AM_indY:
                sprintf(d->_disasm_text[iInst], "%s ($%02x),Y (%d)", inst.mnemonic, lo, lo);
                break;
            case AM_rel:
                sprintf(d->_disasm_text[iInst], "%s $%02x (%d)", inst.mnemonic, lo, lo);
                break;
            case AM_zpg:
                sprintf(d->_disasm_text[iInst], "%s $%02x (%d)", inst.mnemonic, lo, lo);
                break;
            case AM_zpgX:
                sprintf(d->_disasm_text[iInst], "%s $%02x,X (%d)", inst.mnemonic, lo, lo);
                break;
            case AM_zpgY:
                sprintf(d->_disasm_text[iInst], "%s $%02x,Y (%d)", inst.mnemonic, lo, lo);
                break;
        }

        int size = inst_get_size(inst);
        switch (size)
        {
        case 3:
            sprintf(d->_disasm_bytes[iInst], "%02x %02x %02x ", opcode, lo, hi);
            break;
        case 2:
            sprintf(d->_disasm_bytes[iInst], "%02x %02x    ", opcode, lo);
            break;
        case 1:
            sprintf(d->_disasm_bytes[iInst], "%02x       ", opcode);
            break;

        default:
            strcpy(d->_disasm_bytes[iInst], "");
            break;
        }

        iInst++;
        iData += size;
    }
    return (Disassembly){iInst, d->_disasm_text, d->_disasm_bytes, d->_disasm_offsets};
    // return (Disassembly){n, d->_disasm_text, d->_disasm_bytes};
}

u16 disasm_get_alignment(Disassembler *d, u16 offset, int backtrack) {
    // ignore warnings
    d = d;
    backtrack = backtrack;
    return offset;
}