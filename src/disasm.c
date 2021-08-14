#include "headers/disasm.h"
#include "headers/instruction.h"

Disassembler *create_disassembler() {
    Disassembler *d = malloc(sizeof(Disassembler));
    memset(d->_disasm_text, 0, N_MAX_DISASM * N_MAX_TEXT_SIZE);
    memset(d->_disasm_bytes, 0, N_MAX_DISASM * N_MAX_BYTE_SIZE);
}

Disassembly disasm(Disassembler* d, uint8_t *data_aligned, int n) {
    strcpy(d->_disasm_text[0], INSTRUCTIONS[data_aligned[0]].mnemonic);
    sprintf(d->_disasm_bytes[0], "%02X", data_aligned[0]);
    return (Disassembly){1, d->_disasm_text, d->_disasm_bytes};
}