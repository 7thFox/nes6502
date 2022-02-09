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
    bool is_header;
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
    bool performs_jump;
    u8 x;
    u8 y;
    u8 a;
    u8 sp;
    u16 pc_jump;
} ExpectedExecutionInfo;

TestResult compare_execution(InstructionExecutionInfo actual, ExpectedExecutionInfo expected);
InstructionExecutionInfo execute_instruction(u8 *rom_value, size_t rom_size, u8 *ram_value, size_t ram_size, void (*pre_execute)(Cpu6502 *));

// pre-execute's
#define rand_range(lo, hi) ((rand() % (hi - lo + 1)) + lo)
void set_a_00(Cpu6502 *c) { c->a = 0x00; }
void set_a_00_FF(Cpu6502 *c) { c->a = rand_range(0x00, 0xFF); }
void set_a_01(Cpu6502 *c) { c->a = 0x01; }
void set_a_01_7E(Cpu6502 *c) { c->a = rand_range(0x01, 0x7E); }
void set_a_01_7F(Cpu6502 *c) { c->a = rand_range(0x01, 0x7F); }
void set_a_02_7F(Cpu6502 *c) { c->a = rand_range(0x02, 0x7F); }
void set_a_80(Cpu6502 *c) { c->a = 0x80; }
void set_a_80_FE(Cpu6502 *c) { c->a = rand_range(0x80, 0xFE); }
void set_a_80_FF(Cpu6502 *c) { c->a = rand_range(0x80, 0xFF); }
void set_a_81_FF(Cpu6502 *c) { c->a = rand_range(0x81, 0xFF); }
void set_a_7F(Cpu6502 *c) { c->a = 0x7F; }
void set_a_FF(Cpu6502 *c) { c->a = 0xFF; }

void set_sp_00(Cpu6502 *c) { c->sp = 0x00; }
void set_sp_00_FF(Cpu6502 *c) { c->sp = rand_range(0x00, 0xFF); }
void set_sp_01(Cpu6502 *c) { c->sp = 0x01; }
void set_sp_01_7E(Cpu6502 *c) { c->sp = rand_range(0x01, 0x7E); }
void set_sp_01_7F(Cpu6502 *c) { c->sp = rand_range(0x01, 0x7F); }
void set_sp_02_7F(Cpu6502 *c) { c->sp = rand_range(0x02, 0x7F); }
void set_sp_80(Cpu6502 *c) { c->sp = 0x80; }
void set_sp_80_FE(Cpu6502 *c) { c->sp = rand_range(0x80, 0xFE); }
void set_sp_80_FF(Cpu6502 *c) { c->sp = rand_range(0x80, 0xFF); }
void set_sp_81_FF(Cpu6502 *c) { c->sp = rand_range(0x81, 0xFF); }
void set_sp_7F(Cpu6502 *c) { c->sp = 0x7F; }
void set_sp_FF(Cpu6502 *c) { c->sp = 0xFF; }

void set_x_00(Cpu6502 *c) { c->x = 0x00; }
void set_x_00_FF(Cpu6502 *c) { c->x = rand_range(0x00, 0xFF); }
void set_x_01(Cpu6502 *c) { c->x = 0x01; }
void set_x_01_7E(Cpu6502 *c) { c->x = rand_range(0x01, 0x7E); }
void set_x_01_7F(Cpu6502 *c) { c->x = rand_range(0x01, 0x7F); }
void set_x_02_7F(Cpu6502 *c) { c->x = rand_range(0x02, 0x7F); }
void set_x_80(Cpu6502 *c) { c->x = 0x80; }
void set_x_80_FE(Cpu6502 *c) { c->x = rand_range(0x80, 0xFE); }
void set_x_80_FF(Cpu6502 *c) { c->x = rand_range(0x80, 0xFF); }
void set_x_81_FF(Cpu6502 *c) { c->x = rand_range(0x81, 0xFF); }
void set_x_7F(Cpu6502 *c) { c->x = 0x7F; }
void set_x_FF(Cpu6502 *c) { c->x = 0xFF; }

void set_y_00(Cpu6502 *c) { c->y = 0x00; }
void set_y_00_FF(Cpu6502 *c) { c->y = rand_range(0x00, 0xFF); }
void set_y_01(Cpu6502 *c) { c->y = 0x01; }
void set_y_01_7E(Cpu6502 *c) { c->y = rand_range(0x01, 0x7E); }
void set_y_01_7F(Cpu6502 *c) { c->y = rand_range(0x01, 0x7F); }
void set_y_02_7F(Cpu6502 *c) { c->y = rand_range(0x02, 0x7F); }
void set_y_80(Cpu6502 *c) { c->y = 0x80; }
void set_y_80_FE(Cpu6502 *c) { c->y = rand_range(0x80, 0xFE); }
void set_y_80_FF(Cpu6502 *c) { c->y = rand_range(0x80, 0xFF); }
void set_y_81_FF(Cpu6502 *c) { c->y = rand_range(0x81, 0xFF); }
void set_y_7F(Cpu6502 *c) { c->y = 0x7F; }
void set_y_FF(Cpu6502 *c) { c->y = 0xFF; }

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

testcase(DEY_impl__N0Z0) {
    u8 rom_value[] = {
        (u8)0x88,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_02_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
            updates_y: true,
            y: info.y0 - 1,
        });
}

testcase(DEY_impl__N0Z0_boundary) {
    u8 rom_value[] = {
        (u8)0x88,
    };

    return compare_execution(
        execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_80),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
            updates_y: true,
            y: 0x7F,
        });
}

testcase(DEY_impl__N0Z1) {
    u8 rom_value[] = {
        (u8)0x88,
    };

    return compare_execution(
        execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_01),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
            updates_y: true,
            y: 0,
        });
}

testcase(DEY_impl__N1Z0) {
    u8 rom_value[] = {
        (u8)0x88,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_81_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
            updates_y: true,
            y: info.y0 - 1,
        });
}

testcase(DEY_impl__N1Z0_boundary) {
    u8 rom_value[] = {
        (u8)0x88,
    };

    return compare_execution(
        execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_00),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
            updates_y: true,
            y: 0xFF,
        });
}

testcase(INX_impl__N0Z0) {
    u8 rom_value[] = {
        (u8)0xE8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_01_7E);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
            updates_x: true,
            x: info.x0 + 1,
        });
}

testcase(INX_impl__N0Z0_boundary) {
    u8 rom_value[] = {
        (u8)0xE8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_00);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
            updates_x: true,
            x: 0x01,
        });
}

testcase(INX_impl__N0Z1) {
    u8 rom_value[] = {
        (u8)0xE8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
            updates_x: true,
            x: 0x00,
        });
}

testcase(INX_impl__N1Z0) {
    u8 rom_value[] = {
        (u8)0xE8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_80_FE);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
            updates_x: true,
            x: info.x0 + 1,
        });
}

testcase(INX_impl__N1Z0_boundary) {
    u8 rom_value[] = {
        (u8)0xE8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
            updates_x: true,
            x: 0x80,
        });
}

testcase(JMP_abs) {
    u16 jmp_to = rand_range(0x4010, 0xFF00);
    u8 rom_value[] = {
        (u8)0x4C, (u8)(jmp_to & 0xFF), (u8)(jmp_to >> 8),
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 3,
            instruction_size: 3,
            performs_jump: true,
            pc_jump: jmp_to,
        });
}

testcase(LDA_imm__N0Z0) {
    u8 val = rand_range(0x01, 0x7F);
    u8 rom_value[] = {
        (u8)0xA9, val,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_a: true,
            a: val,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(LDA_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0xA9, (u8)0x00,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_a: true,
            a: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(LDA_imm__N1Z0) {
    u8 val = rand_range(0x80, 0xFF);
    u8 rom_value[] = {
        (u8)0xA9, val,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_a: true,
            a: val,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

testcase(LDX_imm__N0Z0) {
    u8 val = rand_range(0x01, 0x7F);
    u8 rom_value[] = {
        (u8)0xA2, val,
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
            x: val,
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
    u8 val = rand_range(0x80, 0xFF);
    u8 rom_value[] = {
        (u8)0xA2, val,
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
            x: val,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

testcase(LDY_imm__N0Z0) {
    u8 val = rand_range(0x01, 0x7F);
    u8 rom_value[] = {
        (u8)0xA0, val,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_y: true,
            y: val,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(LDY_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0xA0, (u8)0x00,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_y: true,
            y: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(LDY_imm__N1Z0) {
    u8 val = rand_range(0x80, 0xFF);
    u8 rom_value[] = {
        (u8)0xA0, val,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 2,
            updates_y: true,
            y: val,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

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

testcase(SEC_impl) {
    u8 rom_value[] = {
        (u8)0x38,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_C_CARRY,
        });
}

testcase(SED_impl) {
    u8 rom_value[] = {
        (u8)0xF8,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_D_DECIMAL,
        });
}

testcase(SEI_impl) {
    u8 rom_value[] = {
        (u8)0x78,
    };

    return compare_execution(
        execute_instruction(
            rom_value, sizeof(rom_value) / sizeof(u8),
            NULL, 0,
            NULL),
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            flags_set: STAT_I_INTERRUPT,
        });
}

testcase(TAX_imm__N0Z0) {
    u8 rom_value[] = {
        (u8)0xAA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_a_01_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_x: true,
            x: info.a0,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(TAX_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0xAA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_a_00);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_x: true,
            x: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(TAX_imm__N1Z0) {
    u8 rom_value[] = {
        (u8)0xAA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_a_80_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_x: true,
            x: info.a0,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

testcase(TAY_imm__N0Z0) {
    u8 rom_value[] = {
        (u8)0xA8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_a_01_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_y: true,
            y: info.a0,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(TAY_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0xA8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_a_00);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_y: true,
            y: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(TAY_imm__N1Z0) {
    u8 rom_value[] = {
        (u8)0xA8,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_a_80_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_y: true,
            y: info.a0,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

testcase(TSX_imm__N0Z0) {
    u8 rom_value[] = {
        (u8)0xBA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_sp_01_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_x: true,
            x: info.sp0,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(TSX_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0xBA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_sp_00);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_x: true,
            x: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(TSX_imm__N1Z0) {
    u8 rom_value[] = {
        (u8)0xBA,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_sp_80_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_x: true,
            x: info.sp0,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

testcase(TXA_imm__N0Z0) {
    u8 rom_value[] = {
        (u8)0x8A,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_01_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_a: true,
            a: info.x0,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(TXA_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0x8A,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_00);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_a: true,
            a: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(TXA_imm__N1Z0) {
    u8 rom_value[] = {
        (u8)0x8A,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_80_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_a: true,
            a: info.x0,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
        });
}

testcase(TXS_imm) {
    u8 rom_value[] = {
        (u8)0x9A,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_x_00_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_sp: true,
            sp: info.x0,
        });
}

testcase(TYA_imm__N0Z0) {
    u8 rom_value[] = {
        (u8)0x98,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_01_7F);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_a: true,
            a: info.y0,
            flags_unset: STAT_N_NEGATIVE | STAT_Z_ZERO,
        });
}

testcase(TYA_imm__N0Z1) {
    u8 rom_value[] = {
        (u8)0x98,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_00);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_a: true,
            a: 0x00,
            flags_set: STAT_Z_ZERO,
            flags_unset: STAT_N_NEGATIVE,
        });
}

testcase(TYA_imm__N1Z0) {
    u8 rom_value[] = {
        (u8)0x98,
    };

    InstructionExecutionInfo info = execute_instruction(
        rom_value, sizeof(rom_value) / sizeof(u8),
        NULL, 0,
        set_y_80_FF);

    return compare_execution(
        info,
        (ExpectedExecutionInfo){
            num_cycles: 2,
            instruction_size: 1,
            updates_a: true,
            a: info.y0,
            flags_set: STAT_N_NEGATIVE,
            flags_unset: STAT_Z_ZERO,
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

// hacky/lazy way to do headers within my test list:
#define header(func_name, text)                \
    testcase(func_name) {                      \
        sprintf(error_message, "%s", text);    \
        return (TestResult){is_header : true}; \
    }

header(__HEADER__MISC__,     "Miscellaneous Instructions");
header(__HEADER__LOAD__,     "Load Instructions");
header(__HEADER__TRANSFER__, "Transfer Instructions");
header(__HEADER__INCDEC__,   "Increment/Decrement Instructions");
header(__HEADER__FLAG__,     "Flag Set/Clear Instructions");

void parse_args(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    enable_stacktrace();
    parse_args(argc, argv);

    TestResult (*test_functions[])() = {

        &__HEADER__FLAG__,
        &CLC_impl,
        &CLD_impl,
        &CLI_impl,
        &CLV_impl,
        &SEC_impl,
        &SED_impl,
        &SEI_impl,

        &__HEADER__LOAD__,
        &LDA_imm__N0Z0,
        &LDA_imm__N0Z1,
        &LDA_imm__N1Z0,
        &LDX_imm__N0Z0,
        &LDX_imm__N0Z1,
        &LDX_imm__N1Z0,
        &LDY_imm__N0Z0,
        &LDY_imm__N0Z1,
        &LDY_imm__N1Z0,

        &__HEADER__TRANSFER__,
        &TAX_imm__N0Z0,
        &TAX_imm__N0Z1,
        &TAX_imm__N1Z0,
        &TAY_imm__N0Z0,
        &TAY_imm__N0Z1,
        &TAY_imm__N1Z0,
        &TSX_imm__N0Z0,
        &TSX_imm__N0Z1,
        &TSX_imm__N1Z0,
        &TXA_imm__N0Z0,
        &TXA_imm__N0Z1,
        &TXA_imm__N1Z0,
        &TXS_imm,
        &TYA_imm__N0Z0,
        &TYA_imm__N0Z1,
        &TYA_imm__N1Z0,

        &__HEADER__INCDEC__,

        &INX_impl__N0Z0,
        &INX_impl__N0Z0_boundary,
        &INX_impl__N0Z1,
        &INX_impl__N1Z0,
        &INX_impl__N1Z0_boundary,

        &DEX_impl__N0Z0,
        &DEX_impl__N0Z0_boundary,
        &DEX_impl__N0Z1,
        &DEX_impl__N1Z0,
        &DEX_impl__N1Z0_boundary,

        &DEY_impl__N0Z0,
        &DEY_impl__N0Z0_boundary,
        &DEY_impl__N0Z1,
        &DEY_impl__N1Z0,
        &DEY_impl__N1Z0_boundary,

        &__HEADER__MISC__,

        &NOP_impl,
        &JMP_abs,
    };

    char buff[64];
    int n_tests = sizeof(test_functions) / (sizeof(test_functions[0]));

    printf("\n");

    const int CLOCKS_PER_MS = (CLOCKS_PER_SEC / 1000);

    bool all_success = true;
    clock_t start_all = clock();
    for (int test_index = 0; test_index < n_tests; test_index++)
    {
        TestResult(*test)() = test_functions[test_index];
        get_test_name(buff, test);

        clock_t start_test = clock();
        TestResult result = test();
        clock_t end_test = clock();

        if (result.is_header) {
            printf("%s:\n", error_message);
            continue;
        }

        printf("  %-25s", buff);
        if (result.is_success) {
            printf(" Success");
        }
        else {
            all_success = false;
            printf(" Failed");
        }

        int runtime_clocks = end_test - start_test;
        int runtime_ms = runtime_clocks / CLOCKS_PER_MS;
        printf(" (%ims", runtime_ms);
        if (runtime_ms == 0) {
            printf(" %i ticks", runtime_clocks);
        }
        printf(")");

        if (!result.is_success) {
            printf(" - %s", error_message);
        }
        printf("\n");
    }

    clock_t end_all = clock();

    if (all_success)
    {
        printf("All unit tests completed successfully. Total run time: ");
    }
    else {
        printf("Unit tests failed. Total run time: ");
    }

    int total_runtime_clocks = end_all - start_all;
    int total_runtime_ms = total_runtime_clocks / CLOCKS_PER_MS;
    printf("%ims", total_runtime_ms);
    if (total_runtime_ms == 0) {
        printf(" %i ticks", total_runtime_clocks);
    }
    printf("\n");

    return all_success ? 0 : 1;
}

#define arg(flag, n_args, block)      \
    if (strcmp(argv[i], flag) == 0 && \
        i + n_args < argc)            \
    {                                 \
        block;                        \
        i += n_args;                  \
        continue;                     \
    }

void parse_args(int argc, char* argv[]) {
    unsigned int seed = time(NULL);

    for (int i = 1; i < argc; i++) {
        arg("--seed", 1,
            {
                seed = (unsigned int)atoi(argv[i + 1]);
            });
    }

    printf("rand seed: %i\n", seed);
    srand(seed);

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
    if (expected.performs_jump) {
        assert_equals(
            expected.pc_jump,
            actual.pc1,
            "Program Counter (jump)");
    }
    else {
        assert_equals(
            actual.pc0 + expected.instruction_size,
            actual.pc1,
            "Program Counter");
    }
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
