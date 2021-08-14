#include "headers/disasm.h"

Disassembler *create_disassembler() {
    Disassembler *d = malloc(sizeof(Disassembler));
    memset(d->_disasm_text, 0, N_MAX_DISASM * N_MAX_TEXT_SIZE);
    // memset(d->_disasm_bytes, 0, N_MAX_DISASM * N_MAX_BYTE_SIZE);
}

Disassembly disasm(Disassembler* d, uint8_t *data_aligned, size_t data_size, int n) {
    debugf("Getting %i instructions\n", n);
    if (n > N_MAX_DISASM) n = N_MAX_DISASM;
    int iData = 0;
    int iInst = 0;
    while (iInst < n && iData < data_size) {
        uint8_t opcode = data_aligned[iData];
        uint8_t lo = iData + 1 < data_size ? data_aligned[iData+1] : 0;
        uint8_t hi = iData + 2 < data_size ? data_aligned[iData+2] : 0;
        uint16_t param16 = (hi << 8) | lo;
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

uint16_t disasm_get_alignment(Disassembler *d, uint16_t offset, int backtrack) {
    return offset;
}