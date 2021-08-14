#include "headers/disasm.h"

Disassembler *create_disassembler() {
    Disassembler *d = malloc(sizeof(Disassembler));
    memset(d->_disasm_text, 0, N_MAX_DISASM * N_MAX_TEXT_SIZE);
    // memset(d->_disasm_bytes, 0, N_MAX_DISASM * N_MAX_BYTE_SIZE);
}

Disassembly disasm(Disassembler* d, uint8_t *data_aligned, size_t data_size, int n) {
    if (n > N_MAX_DISASM) n = N_MAX_DISASM;
    int iData = 0;
    for (int iInst = 0; iInst < n && iData < data_size ; iInst++) {
        uint8_t opcode = data_aligned[iData];
        InstructionMeta inst = INSTRUCTIONS[opcode];

        strncpy(d->_disasm_text[iInst], inst.mnemonic, 3);
        // tracef("sprintf\n", opcode, opcode);
        // char *byte = d->_disasm_bytes[iInst];
        // strncpy(d->_disasm_bytes[iInst], inst.mnemonic, 3);
        // snprintf(d->_disasm_bytes[iInst], 2, "%02X", opcode);

        iData += inst_get_size(inst);
    }
    return (Disassembly){n, d->_disasm_text, 0};
    // return (Disassembly){n, d->_disasm_text, d->_disasm_bytes};
}

uint16_t disasm_get_alignment(Disassembler *d, uint16_t offset, int backtrack) {
    return offset;
}