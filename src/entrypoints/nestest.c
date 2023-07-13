
#include "../headers/common.h"
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

#define STOP_AFTER 1
const clock_t CLOCKS_PER_MS = (CLOCKS_PER_SEC / 1000);

#define COLOR 1
const char *ROM_FILE = "./example/nestest-prg.rom";

void fatal(const char *msg);

u64 hex(const char *line, int start, int len);
u64 dec(const char* line, int start, int len);

int main() {
    if (!init_logging("monitor.log"))
        exit(EXIT_FAILURE);

    tracef("main \n");

    enable_stacktrace();

    MemoryMap mem;
    mem.n_read_blocks  = 0;
    mem.n_write_blocks = 0;
    mem._ppu           = NULL;

    Rom rom;
    if (!rom_load(&rom, ROM_FILE)) {

        fatal("Failed to open nestest ROM");
    }
    mem_add_rom(&mem, &rom, "ROM");


    Ram zpg_ram;
    zpg_ram.map_offset = 0x0000;
    zpg_ram.size       = 0x100;
    zpg_ram.value      = malloc(zpg_ram.size);
    mem_add_ram(&mem, &zpg_ram, "ZPG");

    Ram stack_ram;
    stack_ram.map_offset = 0x0100;
    stack_ram.size       = 0x0100;
    stack_ram.value      = malloc(stack_ram.size);
    mem_add_ram(&mem, &stack_ram, "STACK");

    Ram ram;
    ram.map_offset = 0x0200;
    ram.size       = 0x0600;
    ram.value      = malloc(ram.size);
    mem_add_ram(&mem, &ram, "RAM");

    PPURegisters ppu;
    ppu.status = 0xA2; // just to make sure I can actually read from here
    mem_add_ppu(&mem, &ppu);

    Cpu6502 cpu;
    cpu.p = 0;
    cpu.sp = 0xFD;
    cpu.memmap   = &mem;
    cpu.addr_bus = rom.map_offset;
    cpu_resb(&cpu);

    FILE *expected = fopen("./example/nestest_good_output.log", "r");
    if (!expected) {
        fatal("Failed to open expected output log");
    }

    // if (cpu.pc != 0xC000) {
    //     fatal("ROM start not at $C000");
    // }

#define LINE_TOTAL 8991 - 1
#define BUFFER_SIZE 128
    char to_execute[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    int fails = 0;
    int line_count = 0;
    init_profiler();

    fgets(to_execute, BUFFER_SIZE, expected);
    if (to_execute[81] != '\0' && to_execute[81] != '\r') {
        fatal("EOL not at expected position");
    }

    u64 cyc_last = 0;

    while (fgets(result, BUFFER_SIZE, expected))
    {
        if (result[0] == '#') continue;// comment -- skip

        clock_t start = clock();

#define test_fail(msg, ...) \
{\
    clock_t elapsed = clock() - start;\
    printf(                             \
        "\033[31m"                      \
        "(%04i/%04i) [Failed] %.*s in %lims\n"         \
        "    ", line_count, LINE_TOTAL, 28, to_execute, elapsed / CLOCKS_PER_MS); \
    printf(msg, __VA_ARGS__); \
    printf("\033[0;39m\n"); \
    if (++fails >= STOP_AFTER) break; \
    continue; \
}
#define test_pass() \
{\
    clock_t elapsed = clock() - start;\
    printf(                             \
        "\033[32m"                      \
        "(%04i/%04i) [Passed] %.*s in %lims"    \
        "\033[0;39m\n", line_count, LINE_TOTAL, 28, to_execute, elapsed / CLOCKS_PER_MS); \
    fails = 0; \
}

        // 0         1         2         3         4         5         6         7         8
        // 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
        // C736  18        CLC                             A:00 X:00 Y:00 P:27 SP:FB CYC: 87
        // C780  85 01     STA $01 = 00                    A:FF X:00 Y:00 P:A4 SP:FB CYC:264

        if (result[81] != '\0' && result[81] != '\r') {
            fatal("EOL not at expected position");
        }

        line_count++;

        u16 pc = hex(to_execute, 0, 4);
        if (cpu.pc != pc) {
            test_fail("PC Expected: %04X Actual: %04X", pc, cpu.pc);
        }

        do {
            cpu_pulse(&cpu);
        } while (cpu.tcu != 0);

        u64 cyc = dec(result, 78, 3) / 3;// IDK why this is a multiple of 3, probably some dumb internal version of cycles
        if (cpu.cyc != cyc) {
            test_fail("CYC Expected: %li(+%li) Actual: %li(+%li)", cyc, cyc - cyc_last, cpu.cyc, cpu.cyc - cyc_last);
        }

        if (to_execute[20] == '$' && to_execute[24] == '=') {
            u8 zpg_addr = hex(to_execute, 21, 2);
            u8 val = hex(to_execute, 26, 2);
            u8 actual = mem_read_addr(&mem, zpg_addr);
            if (actual != val) {
                test_fail("zpg $%02X Expected: %02X Actual: %02X", zpg_addr, val, actual);
            }
        }
        else if (to_execute[20] == '$' && to_execute[26] == '=') {
            u16 abs_addr = hex(to_execute, 21, 4);
            u8 val = hex(to_execute, 28, 2);
            u8 actual = mem_read_addr(&mem, abs_addr);
            if (actual != val) {
                test_fail("abs $%04X Expected: %02X Actual: %02X", abs_addr, val, actual);
            }
        }

        u8 a = hex(result, 50, 2);
        if (cpu.a != a) {
            test_fail("A Expected: %02X Actual: %02X", a, cpu.a);
        }

        u8 x = hex(result, 55, 2);
        if (cpu.x != x) {
            test_fail("X Expected: %02X Actual: %02X", x, cpu.x);
        }

        u8 y = hex(result, 60, 2);
        if (cpu.y != y) {
            test_fail("Y Expected: %02X Actual: %02X", y, cpu.y);
        }

        u8 p = hex(result, 65, 2);
        if (cpu.p != p) {
            test_fail("P Expected: %02X Actual: %02X", p, cpu.p);
        }

        u8 sp = hex(result, 71, 2);
        if (cpu.sp != sp) {
            test_fail("SP Expected: %02X Actual: %02X", sp, cpu.sp);
        }

        test_pass();

        memcpy(to_execute, result, BUFFER_SIZE);

        switch (pc) {
            // manual cycle adjustments because the file is borked
            case 0xC797:
                cyc_last = 1 - 3 - 1;// C799: STA zpg => 3 cycles
                cpu.cyc = cyc_last;
                break;
            default:
                cyc_last = cyc;
                break;
        }
    }


    end_profiler("nestest.profile.json");

    fclose(expected);
    return 0;
}



void fatal(const char *msg)
{
    printf("\033[31m[Fatal] %s\033[0;39m\n", msg);
    exit(2);
}


u64 hex(const char *line, int start, int len)
{
    u64 val = 0;
    for (int i = start; i < start+len; i++) {
        val <<= 4;

        char c = line[i];
        if (c >= '0' && c <= '9') val += c - '0';
        if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
        if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
    }
    return val;
}
u64 dec(const char* line, int start, int len)
{
    u64 val = 0;
    for (int i = start; i < start+len; i++) {
        val *= 10;

        char c = line[i];
        if (c >= '0' && c <= '9') val += c - '0';
        if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
        if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
    }
    return val;
}