
#include "../headers/cpu6502.h"
#include "../headers/disasm.h"
#include "../headers/log.h"
#include "../headers/ram.h"
#include "../headers/rom.h"
#include "execinfo.h"
#include "ncurses.h"
#include "signal.h"
#include "stdio.h"
#include "time.h"
#include "../headers/profile.h"
#include <stddef.h>
#include <stdint.h>

const char *ROM_FILE = "./example/nestest-prg.rom";

int main() {
    init_profiler();
    tracef("main \n");

    enable_stacktrace();

    Rom rom;
    if (!rom_load(&rom, ROM_FILE)) {
        fprintf(stderr, "Failed parsing rom file.\n");
        return 1;
    }

    Disassembler *dis = create_disassembler();

    size_t offset = (0xC000 - rom.map_offset);

    while (offset < rom.rom_size)
    {
        Disassembly d = disasm(dis, rom.value + offset, rom.rom_size - offset, INT32_MAX);

        if (d.countBytes == 0) {
            break;
        }

        for (int i = 0; i < d.countInst; i++) {
            u16 addr = rom.map_offset + offset + d.offsets[i];
            printf("$%04X: %s %s\n", addr, d.bytes[i], d.text[i]);
        }
        offset += d.countBytes;
    }
}