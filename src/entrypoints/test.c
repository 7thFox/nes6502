#include "stdio.h"
#include "signal.h"
#include "time.h"
#include "ncurses.h"
#include "execinfo.h"
#include "../headers/log.h"
#include "../headers/ram.h"
#include "../headers/rom.h"
#include "../headers/cpu6502.h"
#include "../headers/disasm.h"

#define testcase(name) TestResult name()
#define assert(cond, msg)                                     \
    if (!(cond))                                              \
    {                                                         \
        char *msg_dyn = malloc(sizeof(msg) / sizeof(msg[0])); \
        strcpy(msg_dyn, msg);                                 \
        return (TestResult){                                  \
            is_success : false,                               \
            failed_message : msg_dyn,                         \
        };                                                    \
    }

typedef struct {
    bool is_success;
    char *failed_message;
} TestResult;

#define setup_cpu(rom_value)                           \
    Cpu6502 cpu;                                       \
    {                                                  \
        MemoryMap mem;                                 \
        mem.n_read_blocks = 0;                         \
        mem.n_write_blocks = 0;                        \
        mem._ppu = NULL;                               \
                                                       \
        Rom rom;                                       \
        rom.value = rom_value;                         \
        rom.rom_size = sizeof(rom_value) / sizeof(u8); \
        rom.map_offset = 0x0000;                       \
        mem_add_rom(&mem, &rom, "ROM");                \
                                                       \
        cpu.memmap = &mem;                             \
        cpu.addr_bus = rom.map_offset;                 \
        cpu_resb(&cpu);                                \
        cpu.pc = 0x0000;                               \
        cpu.addr_bus = 0x0000;                         \
    }

testcase(NOP_impl) {
    u8 rom_value[] = {
        (u8)0xEA,
    };

    setup_cpu(rom_value);
    cpu.x = 0;
    cpu.y = 0;
    cpu.a = 0;
    cpu.sp = 0;
    cpu.p = 0;

    cpu_pulse(&cpu);
    assert(cpu.tcu == 1, "Operation completed in < 2 cycles");
    cpu_pulse(&cpu);
    assert(cpu.tcu == 0, "Operation completed in > 2 cycles");
    assert(cpu.x == 0, "Value of X register changed");
    assert(cpu.y == 0, "Value of Y register changed");
    assert(cpu.a == 0, "Value of Accumulator changed");
    assert(cpu.p == 0, "Value of Status register changed");
    assert(cpu.sp == 0, "Value of Stack Pointer changed");

    return (TestResult) {
        is_success : true,
    };
}

#define _LDX(rom_value, result_value, set_flags, unset_flags)                                          \
    {                                                                                                  \
        setup_cpu(rom_value);                                                                          \
        cpu.x = 0;                                                                                     \
        cpu.p &= ~(unset_flags);                                                                       \
        cpu.p |= set_flags;                                                                            \
                                                                                                       \
        u8 p_expected = (cpu.p & ~(unset_flags)) | set_flags;                                          \
        cpu_pulse(&cpu);                                                                               \
        assert(cpu.tcu == 1, "Operation completed in < 2 cycles");                                     \
        cpu_pulse(&cpu);                                                                               \
        assert(cpu.tcu == 0, "Operation completed in > 2 cycles");                                     \
        assert(cpu.x == result_value, "Value of X register incorrect");                                \
        assert((cpu.p & STAT_N_NEGATIVE) == (p_expected & STAT_N_NEGATIVE), "N flag set incorrectly"); \
        assert((cpu.p & STAT_Z_ZERO) == (p_expected & STAT_Z_ZERO), "Z flag set incorrectly");         \
        assert(cpu.p == p_expected, "Unexpected status flags changed");                                \
    }

testcase(LDX_imm__N0Z0) {
    u8 rom_value[] = {
        (u8)0xA2, (u8)0x1A,
    };

    _LDX(rom_value, 0x1A, 0x00, STAT_N_NEGATIVE | STAT_Z_ZERO);

    return (TestResult) {
        is_success : true,
    };
}

testcase(LDX_imm__N1Z0) {
    u8 rom_value[] = {
        (u8)0xA2, (u8)0x80,
    };

    _LDX(rom_value, 0x80, STAT_N_NEGATIVE, STAT_Z_ZERO);

    return (TestResult) {
        is_success : true,
    };
}

testcase(LDX_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0xA2, (u8)0x00,
    };

    _LDX(rom_value, 0x00, STAT_Z_ZERO, STAT_N_NEGATIVE);

    return (TestResult) {
        is_success : true,
    };
}


    // Rom rom;
    // if (!rom_load(&rom, ROM_FILE)) {
    //     fprintf(stderr, "Failed parsing rom file.\n");
    //     return 1;
    // }
    // mem_add_rom(&mem, &rom, "ROM");

    // Ram zpg_ram;
    // zpg_ram.map_offset = 0x0000;
    // zpg_ram.size = 0x100;
    // zpg_ram.value = malloc(zpg_ram.size);
    // mem_add_ram(&mem, &zpg_ram, "ZPG");

    // Ram stack_ram;
    // stack_ram.map_offset = 0x0100;
    // stack_ram.size = 0x0100;
    // stack_ram.value = malloc(stack_ram.size);
    // mem_add_ram(&mem, &stack_ram, "STACK");

    // Ram ram;
    // ram.map_offset = 0x0200;
    // ram.size = 0x0600;
    // ram.value = malloc(ram.size);
    // mem_add_ram(&mem, &ram, "RAM");

    // PPURegisters ppu;
    // ppu.status = 0xA2;// just to make sure I can actually read from here
    // mem_add_ppu(&mem, &ppu);



void get_test_name(char* buff, void *test_func) {
    void *bt[1] = { test_func };
    char **b = backtrace_symbols(bt, 1);
    if (b) {
        int start = 0;
        while (b[0][start] != '(') start++;

        int end = start;
        while (b[0][end] != '+') end++;

        int len = end - start - 1;
        sprintf(buff, "%.*s", len, b[0] + start + 1);
        free(b);
    }
    else {
        buff[0] = '\0';
    }
}

// int main(int argc, char* argv[]) {
int main() {
    enable_stacktrace();

    testcase((*test_functions[])) = {
        &NOP_impl,
        &LDX_imm__N0Z0,
        &LDX_imm__N1Z0,
        &LDX_imm__N0Z1,
    };

    char buff[64];
    int n_tests = sizeof(test_functions) / (sizeof(test_functions[0]));

    bool all_success = true;
    clock_t start_all = clock();
    for (int test_index = 0; test_index < n_tests; test_index++)
    {
        testcase((*test)) = test_functions[test_index];
        get_test_name(buff, test);
        printf("Running test %i/%i '%s'...\r", test_index+1, n_tests, buff);

        clock_t start_test = clock();
        TestResult result = test();
        clock_t end_test = clock();

        int runtime_ms = (end_test - start_test) / CLOCKS_PER_SEC * 1000;

        if (result.is_success) {
            printf("  %-20s Success (%ims)                             \n", buff, runtime_ms);
        }
        else {
            all_success = false;
            if (result.failed_message) {
                printf("  %-20s Failed   (%ims) - %s                       \n", buff, runtime_ms, result.failed_message);
                free(result.failed_message);
            }
            else {
                printf("  %-20s Failed   (%ims)                            \n", buff, runtime_ms);
            }
        }
    }

    clock_t end_all = clock();
    int total_runtime_ms = (end_all - start_all) / CLOCKS_PER_SEC * 1000;

    printf("\n");
    if (all_success)
    {
        printf("All unit tests completed successfully. Total run time: %ims\n", total_runtime_ms);
    }
    else {
        printf("Unit tests failed. Total run time: %ims\n", total_runtime_ms);
    }
}
