#include "stdio.h"
#include "signal.h"
#include "time.h"
#include "stdlib.h"
#include "execinfo.h"
#include "../headers/log.h"
#include "../headers/ram.h"
#include "../headers/rom.h"
#include "../headers/cpu6502.h"
#include "../headers/disasm.h"

#define MAX_CYCLES_PER_OP 6
#define RAM_OFFSET 0x0000
#define ROM_OFFSET 0x4000

char error_message[256];// I'd prefer not to malloc/free for every test

// #define TEST_ALL_TESTS_INCLUDED

#ifdef TEST_ALL_TESTS_INCLUDED
// static functions are the only ones that error for -Wunused-function, but
// they also don't provide info when using stacktrace(), which we use for
// test names.
#define testcase(name) static TestResult name()
#else
#define testcase(name) TestResult name()
#endif

typedef struct {
    bool is_success;
} TestResult;

typedef struct {
    // general information
    int num_cycles;

    // initial values
    u16 pc0;
    u8 x0;
    u8 y0;
    u8 a0;
    u8 sp0;
    u8 p0;

    // resulting values
    u16 pc1;
    u8 x1;
    u8 y1;
    u8 a1;
    u8 sp1;
    u8 p1;
    u8 bit_fields1;
    u16 addr_bus1;

} InstructionExecutionInfo;

typedef struct {
    int num_cycles;
    int instruction_size;

    u8 flags_set;
    u8 flags_unset;

    bool updates_x;
    bool updates_y;
    bool updates_a;
    bool updates_sp;
    u8 x;
    u8 y;
    u8 a;
    u8 sp;
} ExpectedExecutionInfo;

TestResult compare_execution(InstructionExecutionInfo actual, ExpectedExecutionInfo expected);
InstructionExecutionInfo execute_instruction(u8 *rom_value, size_t rom_size, u8 *ram_value, size_t ram_size, void (*pre_execute)(Cpu6502 *));

// pre-execute's
void set_x_00(Cpu6502 *c);
void set_x_01(Cpu6502 *c);
void set_x_02_7F(Cpu6502 *c);
void set_x_80(Cpu6502 *c);
void set_x_81_FF(Cpu6502 *c);

testcase(NOP_impl) {
    u8 rom_value[] = {
        (u8)0xEA,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
        });
}

testcase(LDX_imm__N0Z0) {
    u8 rom_value[] = {
        (u8)0xA2, (u8)0x11,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_x: true,
            x: 0x11,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(LDX_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0xA2, (u8)0x00,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_x: true,
            x: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(LDX_imm__N1Z0) {
    u8 rom_value[] = {
        (u8)0xA2, (u8)0x81,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_x: true,
            x: 0x81,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

testcase(CLC_impl) {
    u8 rom_value[] = {
        (u8)0x18,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_C_CARRY,
        });
}

testcase(CLD_impl) {
    u8 rom_value[] = {
        (u8)0xD8,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_D_DECIMAL,
        });
}

testcase(CLI_impl) {
    u8 rom_value[] = {
        (u8)0x58,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_I_INTERRUPT,
        });
}

testcase(CLV_impl) {
    u8 rom_value[] = {
        (u8)0xB8,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_V_OVERFLOW,
        });
}

testcase(DEX_impl__N0Z0) {
    u8 rom_value[] = {
        (u8)0xCA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_02_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
            updates_x: true,
            x: info.x0 - 1,
        });
}

testcase(DEX_impl__N0Z0_boundary) {
    u8 rom_value[] = {
        (u8)0xCA,
    };

    return compare_execution(
        execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_80),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
            updates_x: true,
            x: 0x7F,
        });
}

testcase(DEX_impl__N0Z1) {
    u8 rom_value[] = {
        (u8)0xCA,
    };

    return compare_execution(
        execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_01),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
            updates_x: true,
            x: 0,
        });
}

testcase(DEX_impl__N1Z0) {
    u8 rom_value[] = {
        (u8)0xCA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_81_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
            updates_x: true,
            x: info.x0 - 1,
        });
}

testcase(DEX_impl__N1Z0_boundary) {
    u8 rom_value[] = {
        (u8)0xCA,
    };

    return compare_execution(
        execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_00),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
            updates_x: true,
            x: 0xFF,
        });
}

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

    unsigned int seed = time(NULL);
    printf("rand seed: %i\n", seed);
    srand(seed);

    TestResult (*test_functions[])() = {
        &NOP_impl,

        // clear-flag instructions
        &CLC_impl,
        &CLD_impl,
        &CLI_impl,
        &CLV_impl,

        // load instructions
        &LDX_imm__N0Z0,
        &LDX_imm__N0Z1,
        &LDX_imm__N1Z0,

        // Increment/Decrement
        &DEX_impl__N0Z0,
        &DEX_impl__N0Z0_boundary,
        &DEX_impl__N0Z1,
        &DEX_impl__N1Z0,
        &DEX_impl__N1Z0_boundary,
    };

    char buff[64];
    int n_tests = sizeof(test_functions) / (sizeof(test_functions[0]));

    bool all_success = true;
    clock_t start_all = clock();
    for (int test_index = 0; test_index < n_tests; test_index++)
    {
        TestResult(*test)() = test_functions[test_index];
        get_test_name(buff, test);
        printf("Running test %i/%i '%s'...", test_index+1, n_tests, buff);

        clock_t start_test = clock();
        TestResult result = test();
        clock_t end_test = clock();

        int runtime_ms = (end_test - start_test) / CLOCKS_PER_SEC * 1000;

        if (result.is_success) {
            printf("\r  %-25s Success (%ims)                             \n", buff, runtime_ms);
        }
        else {
            all_success = false;
            printf("\r  %-25s Failed  (%ims) - %s                       \n", buff, runtime_ms, error_message);
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

#define rand_range(lo, hi) ((rand() % (hi - lo + 1)) + lo)

void set_x_00(Cpu6502 *c) {
    c->x = 0x00;
}
void set_x_01(Cpu6502 *c) {
    c->x = 0x01;
}
void set_x_02_7F(Cpu6502 *c) {
    c->x = rand_range(0x02, 0x7F);
}
void set_x_80(Cpu6502 *c) {
    c->x = 0x80;
}
void set_x_81_FF(Cpu6502 *c) {
    c->x = rand_range(0x81, 0xFF);
}

InstructionExecutionInfo execute_instruction(
    u8 *rom_value, size_t rom_size,
    u8 *ram_value, size_t ram_size,
    void(*pre_execute)(Cpu6502*))
{
    MemoryMap mem;
    mem.n_read_blocks = 0;
    mem.n_write_blocks = 0;
    mem._ppu = NULL;

    Rom rom;
    rom.value = rom_value;
    rom.rom_size = rom_size;
    rom.map_offset = ROM_OFFSET;
    mem_add_rom(&mem, &rom, "ROM");

    Ram ram;
    if (ram_value) {
        ram.value = ram_value;
        ram.size = ram_size;
        ram.map_offset = RAM_OFFSET;
        mem_add_ram(&mem, &ram, "RAM");
    }

    Cpu6502 cpu;
    cpu.memmap = &mem;
    cpu.addr_bus = rom.map_offset;
    cpu_resb(&cpu);// just to do any internal setup before I mess around with all the registers
    cpu.pc = ROM_OFFSET;
    cpu.addr_bus = ROM_OFFSET;
    cpu.x = rand() % 0xFF;
    cpu.y = rand() % 0xFF;
    cpu.a = rand() % 0xFF;
    cpu.sp = rand() % 0xFF;
    cpu.p = rand() % 0xFF;
    // shouldn't matter, but that's why I'm rand()ing them
    cpu.ir = rand() % 0xFF;
    cpu.pd = rand() % 0xFF;
    cpu.data_bus = rand() % 0xFF;

    if (pre_execute) pre_execute(&cpu);

    InstructionExecutionInfo info;
    info.pc0 = cpu.pc;
    info.x0 = cpu.x;
    info.y0 = cpu.y;
    info.a0 = cpu.a;
    info.sp0 = cpu.sp;
    info.p0 = cpu.p;

    int cycles = 0;
    do {
        cpu_pulse(&cpu);
        cycles++;
    } while (cpu.tcu != 0 && cycles < MAX_CYCLES_PER_OP);

    info.num_cycles = cycles;
    info.pc1 = cpu.pc;
    info.x1 = cpu.x;
    info.y1 = cpu.y;
    info.a1 = cpu.a;
    info.sp1 = cpu.sp;
    info.p1 = cpu.p;
    info.bit_fields1 = cpu.bit_fields;
    info.addr_bus1 = cpu.addr_bus;

    return info;
}

#define assert_equals(expected, actual, value_name)         \
    if ((actual) != (expected))                             \
    {                                                       \
        sprintf(error_message,                              \
                "%s: Expected $%02x (%i), got $%02x (%i).", \
                value_name,                                 \
                expected, expected,                         \
                actual, actual);                            \
        return (TestResult){is_success : false};            \
    }

TestResult compare_execution(
    InstructionExecutionInfo actual,
    ExpectedExecutionInfo expected)
{
    assert_equals(expected.num_cycles, actual.num_cycles, "Cycles");
    assert_equals(
        actual.pc0 + expected.instruction_size,
        actual.pc1,
        "Program Counter");
    assert_equals(
        actual.pc1,
        actual.addr_bus1,
        "Address Bus");
    assert_equals(
        PIN_READ,
        actual.bit_fields1 & PIN_READ,
        "R/W pin");

    if (expected.updates_x) {
        assert_equals(expected.x, actual.x1, "Register X");
    } else {
        assert_equals(actual.x0, actual.x1, "Register X (unchanged)");
    }

    if (expected.updates_y) {
        assert_equals(expected.y, actual.y1, "Register Y");
    } else {
        assert_equals(actual.y0, actual.y1, "Register Y (unchanged)");
    }

    if (expected.updates_a) {
        assert_equals(expected.a, actual.a1, "Register A");
    } else {
        assert_equals(actual.a0, actual.a1, "Register A (unchanged)");
    }

    if (expected.updates_sp) {
        assert_equals(expected.sp, actual.sp1, "Register SP");
    } else {
        assert_equals(actual.sp0, actual.sp1, "Register SP (unchanged)");
    }


    if ((expected.flags_set & STAT_C_CARRY) == STAT_C_CARRY) {
        assert_equals(STAT_C_CARRY, actual.p1 & STAT_C_CARRY, "Flag C");
    } else if ((expected.flags_unset & STAT_C_CARRY) == STAT_C_CARRY) {
        assert_equals(0, actual.p1 & STAT_C_CARRY, "Flag C");
    } else {
        assert_equals(actual.p0 & STAT_C_CARRY, actual.p1 & STAT_C_CARRY, "Flag C (unchanged)");
    }

    if ((expected.flags_set & STAT_Z_ZERO) == STAT_Z_ZERO) {
        assert_equals(STAT_Z_ZERO, actual.p1 & STAT_Z_ZERO, "Flag Z");
    } else if ((expected.flags_unset & STAT_Z_ZERO) == STAT_Z_ZERO) {
        assert_equals(0, actual.p1 & STAT_Z_ZERO, "Flag Z");
    } else {
        assert_equals(actual.p0 & STAT_Z_ZERO, actual.p1 & STAT_Z_ZERO, "Flag Z (unchanged)");
    }

    if ((expected.flags_set & STAT_I_INTERRUPT) == STAT_I_INTERRUPT) {
        assert_equals(STAT_I_INTERRUPT, actual.p1 & STAT_I_INTERRUPT, "Flag I");
    } else if ((expected.flags_unset & STAT_I_INTERRUPT) == STAT_I_INTERRUPT) {
        assert_equals(0, actual.p1 & STAT_I_INTERRUPT, "Flag I");
    } else {
        assert_equals(actual.p0 & STAT_I_INTERRUPT, actual.p1 & STAT_I_INTERRUPT, "Flag I (unchanged)");
    }

    if ((expected.flags_set & STAT_D_DECIMAL) == STAT_D_DECIMAL) {
        assert_equals(STAT_D_DECIMAL, actual.p1 & STAT_D_DECIMAL, "Flag D");
    } else if ((expected.flags_unset & STAT_D_DECIMAL) == STAT_D_DECIMAL) {
        assert_equals(0, actual.p1 & STAT_D_DECIMAL, "Flag D");
    } else {
        assert_equals(actual.p0 & STAT_D_DECIMAL, actual.p1 & STAT_D_DECIMAL, "Flag D (unchanged)");
    }

    if ((expected.flags_set & STAT_B_BREAK) == STAT_B_BREAK) {
        assert_equals(STAT_B_BREAK, actual.p1 & STAT_B_BREAK, "Flag B");
    } else if ((expected.flags_unset & STAT_B_BREAK) == STAT_B_BREAK) {
        assert_equals(0, actual.p1 & STAT_B_BREAK, "Flag B");
    } else {
        assert_equals(actual.p0 & STAT_B_BREAK, actual.p1 & STAT_B_BREAK, "Flag B (unchanged)");
    }

    if ((expected.flags_set & STAT___IGNORE) == STAT___IGNORE) {
        assert_equals(STAT___IGNORE, actual.p1 & STAT___IGNORE, "Flag _");
    } else if ((expected.flags_unset & STAT___IGNORE) == STAT___IGNORE) {
        assert_equals(0, actual.p1 & STAT___IGNORE, "Flag _");
    } else {
        assert_equals(actual.p0 & STAT___IGNORE, actual.p1 & STAT___IGNORE, "Flag _ (unchanged)");
    }

    if ((expected.flags_set & STAT_V_OVERFLOW) == STAT_V_OVERFLOW) {
        assert_equals(STAT_V_OVERFLOW, actual.p1 & STAT_V_OVERFLOW, "Flag V");
    } else if ((expected.flags_unset & STAT_V_OVERFLOW) == STAT_V_OVERFLOW) {
        assert_equals(0, actual.p1 & STAT_V_OVERFLOW, "Flag V");
    } else {
        assert_equals(actual.p0 & STAT_V_OVERFLOW, actual.p1 & STAT_V_OVERFLOW, "Flag V (unchanged)");
    }

    if ((expected.flags_set & STAT_N_NEGATIVE) == STAT_N_NEGATIVE) {
        assert_equals(STAT_N_NEGATIVE, actual.p1 & STAT_N_NEGATIVE, "Flag N");
    } else if ((expected.flags_unset & STAT_N_NEGATIVE) == STAT_N_NEGATIVE) {
        assert_equals(0, actual.p1 & STAT_N_NEGATIVE, "Flag N");
    } else {
        assert_equals(actual.p0 & STAT_N_NEGATIVE, actual.p1 & STAT_N_NEGATIVE, "Flag N (unchanged)");
    }

    return (TestResult){is_success : true};
}
