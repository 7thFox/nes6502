#include "headers/cpu6502.h"

void cpu_pulse(Cpu6502 *c) {

}

void cpu_resb(Cpu6502 *c) {
    // PCL = $FFFC
    // PCH = $FFFD
    c->p |= 1 << 5;    // ?
    c->p |= 1 << 3;    // B
    c->p &= ~(1 << 3); // D
    c->p |= 1 << 2;    // I
}