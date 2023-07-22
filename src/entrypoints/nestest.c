
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

struct test_t{
    u8 err_code;
    const char *msg;
};

struct test_section_t{
    u16 addr_start;
    u16 addr_end;
    const char *name;
};

#define GROUP2_ADDR_START 0xD934
#define GROUP3_ADDR_START 0xF2D6
#define GROUP1_LEN (sizeof(tests_group1) / sizeof(struct test_t))
#define GROUP2_LEN (sizeof(tests_group2) / sizeof(struct test_t))
#define GROUP3_LEN (sizeof(tests_group3) / sizeof(struct test_t))

struct test_t tests_group1[] = {
    {0x01, "BCS failed to branch"},
    {0x02, "BCS branched when it shouldn't have"},
    {0x03, "BCC branched when it shouldn't have"},
    {0x04, "BCC failed to branch"},
    {0x05, "BEQ failed to branch"},
    {0x06, "BEQ branched when it shouldn't have"},
    {0x07, "BNE failed to branch"},
    {0x08, "BNE branched when it shouldn't have"},
    {0x09, "BVS failed to branch"},
    {0x0A, "BVC branched when it shouldn't have"},
    {0x0B, "BVC failed to branch"},
    {0x0C, "BVS branched when it shouldn't have"},
    {0x0D, "BPL failed to branch"},
    {0x0E, "BPL branched when it shouldn't have"},
    {0x0F, "BMI failed to branch"},
    {0x10, "BMI branched when it shouldn't have"},
    {0x11, "PHP/flags failure (bits set)"},
    {0x12, "PHP/flags failure (bits clear)"},
    {0x13, "PHP/flags failure (misc bit states)"},
    {0x14, "PLP/flags failure (misc bit states)"},
    {0x15, "PLP/flags failure (misc bit states)"},
    {0x16, "PHA/PLA failure (PLA didn't affect Z and N properly)"},
    {0x17, "PHA/PLA failure (PLA didn't affect Z and N properly)"},
    {0x18, "ORA # failure"},
    {0x19, "ORA # failure"},
    {0x1A, "AND # failure"},
    {0x1B, "AND # failure"},
    {0x1C, "EOR # failure"},
    {0x1D, "EOR # failure"},
    {0x1E, "ADC # failure (overflow/carry problems)"},
    {0x1F, "ADC # failure (decimal mode was turned on)"},
    {0x20, "ADC # failure"},
    {0x21, "ADC # failure"},
    {0x22, "ADC # failure"},
    {0x23, "LDA # failure (didn't set N and Z correctly)"},
    {0x24, "LDA # failure (didn't set N and Z correctly)"},
    {0x25, "CMP # failure (messed up flags)"},
    {0x26, "CMP # failure (messed up flags)"},
    {0x27, "CMP # failure (messed up flags)"},
    {0x28, "CMP # failure (messed up flags)"},
    {0x29, "CMP # failure (messed up flags)"},
    {0x2A, "CMP # failure (messed up flags)"},
    {0x2B, "CPY # failure (messed up flags)"},
    {0x2C, "CPY # failure (messed up flags)"},
    {0x2D, "CPY # failure (messed up flags)"},
    {0x2E, "CPY # failure (messed up flags)"},
    {0x2F, "CPY # failure (messed up flags)"},
    {0x30, "CPY # failure (messed up flags)"},
    {0x31, "CPY # failure (messed up flags)"},
    {0x32, "CPX # failure (messed up flags)"},
    {0x33, "CPX # failure (messed up flags)"},
    {0x34, "CPX # failure (messed up flags)"},
    {0x35, "CPX # failure (messed up flags)"},
    {0x36, "CPX # failure (messed up flags)"},
    {0x37, "CPX # failure (messed up flags)"},
    {0x38, "CPX # failure (messed up flags)"},
    {0x39, "LDX # failure (didn't set N and Z correctly)"},
    {0x3A, "LDX # failure (didn't set N and Z correctly)"},
    {0x3B, "LDY # failure (didn't set N and Z correctly)"},
    {0x3C, "LDY # failure (didn't set N and Z correctly)"},
    {0x3D, "compare(s) stored the result in a register (whoops!)"},
    {0x71, "SBC # failure"},
    {0x72, "SBC # failure"},
    {0x73, "SBC # failure"},
    {0x74, "SBC # failure"},
    {0x75, "SBC # failure"},
    {0x3E, "INX/DEX/INY/DEY did something bad"},
    {0x3F, "INY/DEY messed up overflow or carry"},
    {0x40, "INX/DEX messed up overflow or carry"},
    {0x41, "TAY did something bad (changed wrong regs, messed up flags)"},
    {0x42, "TAX did something bad (changed wrong regs, messed up flags)"},
    {0x43, "TYA did something bad (changed wrong regs, messed up flags)"},
    {0x44, "TXA did something bad (changed wrong regs, messed up flags)"},
    {0x45, "TXS didn't set flags right, or TSX touched flags and it shouldn't have"},
    {0x46, "wrong data popped, or data not in right location on stack"},
    {0x47, "JSR didn't work as expected"},
    {0x48, "RTS/JSR shouldn't have affected flags"},
    {0x49, "RTI/RTS didn't work right when return addys/data were manually pushed"},
    {0x4A, "LSR A  failed"},
    {0x4B, "ASL A  failed"},
    {0x4C, "ROR A  failed"},
    {0x4D, "ROL A  failed"},
    {0x58, "LDA didn't load the data it expected to load"},
    {0x59, "STA didn't store the data where it was supposed to"},
    {0x5A, "ORA failure"},
    {0x5B, "ORA failure"},
    {0x5C, "AND failure"},
    {0x5D, "AND failure"},
    {0x5E, "EOR failure"},
    {0x5F, "EOR failure"},
    {0x60, "ADC failure"},
    {0x61, "ADC failure"},
    {0x62, "ADC failure"},
    {0x63, "ADC failure"},
    {0x64, "ADC failure"},
    {0x65, "CMP failure"},
    {0x66, "CMP failure"},
    {0x67, "CMP failure"},
    {0x68, "CMP failure"},
    {0x69, "CMP failure"},
    {0x6A, "CMP failure"},
    {0x6B, "CMP failure"},
    {0x6C, "SBC failure"},
    {0x6D, "SBC failure"},
    {0x6E, "SBC failure"},
    {0x6F, "SBC failure"},
    {0x70, "SBC failure"},
    {0x76, "LDA didn't set the flags properly"},
    {0x77, "STA affected flags it shouldn't"},
    {0x78, "LDY didn't set the flags properly"},
    {0x79, "STY affected flags it shouldn't"},
    {0x7A, "LDX didn't set the flags properly"},
    {0x7B, "STX affected flags it shouldn't"},
    {0x7C, "BIT failure"},
    {0x7D, "BIT failure"},
    {0x7E, "ORA failure"},
    {0x7F, "ORA failure"},
    {0x80, "AND failure"},
    {0x81, "AND failure"},
    {0x82, "EOR failure"},
    {0x83, "EOR failure"},
    {0x84, "ADC failure"},
    {0x85, "ADC failure"},
    {0x86, "ADC failure"},
    {0x87, "ADC failure"},
    {0x88, "ADC failure"},
    {0x89, "CMP failure"},
    {0x8A, "CMP failure"},
    {0x8B, "CMP failure"},
    {0x8C, "CMP failure"},
    {0x8D, "CMP failure"},
    {0x8E, "CMP failure"},
    {0x8F, "CMP failure"},
    {0x90, "SBC failure"},
    {0x91, "SBC failure"},
    {0x92, "SBC failure"},
    {0x93, "SBC failure"},
    {0x94, "SBC failure"},
    {0x95, "CPX failure"},
    {0x96, "CPX failure"},
    {0x97, "CPX failure"},
    {0x98, "CPX failure"},
    {0x99, "CPX failure"},
    {0x9A, "CPX failure"},
    {0x9B, "CPX failure"},
    {0x9C, "CPY failure"},
    {0x9D, "CPY failure"},
    {0x9E, "CPY failure"},
    {0x9F, "CPY failure"},
    {0xA0, "CPY failure"},
    {0xA1, "CPY failure"},
    {0xA2, "CPY failure"},
    {0xA3, "LSR failure"},
    {0xA4, "LSR failure"},
    {0xA5, "ASL failure"},
    {0xA6, "ASL failure"},
    {0xA7, "ROL failure"},
    {0xA8, "ROL failure"},
    {0xA9, "ROR failure"},
    {0xAA, "ROR failure"},
    {0xAB, "INC failure"},
    {0xAC, "INC failure"},
    {0xAD, "DEC failure"},
    {0xAE, "DEC failure"},
    {0xAF, "DEC failure"},
    {0xB0, "LDA didn't set the flags properly"},
    {0xB1, "STA affected flags it shouldn't"},
    {0xB2, "LDY didn't set the flags properly"},
    {0xB3, "STY affected flags it shouldn't"},
    {0xB4, "LDX didn't set the flags properly"},
    {0xB5, "STX affected flags it shouldn't"},
    {0xB6, "BIT failure"},
    {0xB7, "BIT failure"},
    {0xB8, "ORA failure"},
    {0xB9, "ORA failure"},
    {0xBA, "AND failure"},
    {0xBB, "AND failure"},
    {0xBC, "EOR failure"},
    {0xBD, "EOR failure"},
    {0xBE, "ADC failure"},
    {0xBF, "ADC failure"},
    {0xC0, "ADC failure"},
    {0xC1, "ADC failure"},
    {0xC2, "ADC failure"},
    {0xC3, "CMP failure"},
    {0xC4, "CMP failure"},
    {0xC5, "CMP failure"},
    {0xC6, "CMP failure"},
    {0xC7, "CMP failure"},
    {0xC8, "CMP failure"},
    {0xC9, "CMP failure"},
    {0xCA, "SBC failure"},
    {0xCB, "SBC failure"},
    {0xCC, "SBC failure"},
    {0xCD, "SBC failure"},
    {0xCE, "SBC failure"},
    {0xCF, "CPX failure"},
    {0xD0, "CPX failure"},
    {0xD1, "CPX failure"},
    {0xD2, "CPX failure"},
    {0xD3, "CPX failure"},
    {0xD4, "CPX failure"},
    {0xD5, "CPX failure"},
    {0xD6, "CPY failure"},
    {0xD7, "CPY failure"},
    {0xD8, "CPY failure"},
    {0xD9, "CPY failure"},
    {0xDA, "CPY failure"},
    {0xDB, "CPY failure"},
    {0xDC, "CPY failure"},
    {0xDD, "LSR failure"},
    {0xDE, "LSR failure"},
    {0xDF, "ASL failure"},
    {0xE0, "ASL failure"},
    {0xE1, "ROR failure"},
    {0xE2, "ROR failure"},
    {0xE3, "ROL failure"},
    {0xE4, "ROL failure"},
    {0xE5, "INC failure"},
    {0xE6, "INC failure"},
    {0xE7, "DEC failure"},
    {0xE8, "DEC failure"},
    {0xE9, "DEC failure"},
    {0xEA, "LDA didn't load what it was supposed to"},
    {0xEB, "read location should've wrapped around ffffh to 0000h"},
    {0xEC, "should've wrapped zeropage address"},
    {0xED, "ORA failure"},
    {0xEE, "ORA failure"},
    {0xEF, "AND failure"},
    {0xF0, "AND failure"},
    {0xF1, "EOR failure"},
    {0xF2, "EOR failure"},
    {0xF3, "ADC failure"},
    {0xF4, "ADC failure"},
    {0xF5, "ADC failure"},
    {0xF6, "ADC failure"},
    {0xF7, "ADC failure"},
    {0xF8, "CMP failure"},
    {0xF9, "CMP failure"},
    {0xFA, "CMP failure"},
    {0xFB, "CMP failure"},
    {0xFC, "CMP failure"},
    {0xFD, "CMP failure"},
    {0xFE, "CMP failure"},
};

struct test_t tests_group2[] = {
    {0x01, "SBC failure"},
    {0x02, "SBC failure"},
    {0x03, "SBC failure"},
    {0x04, "SBC failure"},
    {0x05, "SBC failure"},
    {0x06, "STA failure"},
    {0x07, "JMP () data reading didn't wrap properly (this fails on a 65C02)"},
    {0x08, "LDY,X failure"},
    {0x09, "LDY,X failure"},
    {0x0A, "STY,X failure"},
    {0x0B, "ORA failure"},
    {0x0C, "ORA failure"},
    {0x0D, "AND failure"},
    {0x0E, "AND failure"},
    {0x0F, "EOR failure"},
    {0x10, "EOR failure"},
    {0x11, "ADC failure"},
    {0x12, "ADC failure"},
    {0x13, "ADC failure"},
    {0x14, "ADC failure"},
    {0x15, "ADC failure"},
    {0x16, "CMP failure"},
    {0x17, "CMP failure"},
    {0x18, "CMP failure"},
    {0x19, "CMP failure"},
    {0x1A, "CMP failure"},
    {0x1B, "CMP failure"},
    {0x1C, "CMP failure"},
    {0x1D, "SBC failure"},
    {0x1E, "SBC failure"},
    {0x1F, "SBC failure"},
    {0x20, "SBC failure"},
    {0x21, "SBC failure"},
    {0x22, "LDA failure"},
    {0x23, "LDA failure"},
    {0x24, "STA failure"},
    {0x25, "LSR failure"},
    {0x26, "LSR failure"},
    {0x27, "ASL failure"},
    {0x28, "ASL failure"},
    {0x29, "ROR failure"},
    {0x2A, "ROR failure"},
    {0x2B, "ROL failure"},
    {0x2C, "ROL failure"},
    {0x2D, "INC failure"},
    {0x2E, "INC failure"},
    {0x2F, "DEC failure"},
    {0x30, "DEC failure"},
    {0x31, "DEC failure"},
    {0x32, "LDX,Y failure"},
    {0x33, "LDX,Y failure"},
    {0x34, "STX,Y failure"},
    {0x35, "STX,Y failure"},
    {0x36, "LDA failure"},
    {0x37, "LDA failure to wrap properly from ffffh to 0000h"},
    {0x38, "LDA failure, page cross"},
    {0x39, "ORA failure"},
    {0x3A, "ORA failure"},
    {0x3B, "AND failure"},
    {0x3C, "AND failure"},
    {0x3D, "EOR failure"},
    {0x3E, "EOR failure"},
    {0x3F, "ADC failure"},
    {0x40, "ADC failure"},
    {0x41, "ADC failure"},
    {0x42, "ADC failure"},
    {0x43, "ADC failure"},
    {0x44, "CMP failure"},
    {0x45, "CMP failure"},
    {0x46, "CMP failure"},
    {0x47, "CMP failure"},
    {0x48, "CMP failure"},
    {0x49, "CMP failure"},
    {0x4A, "CMP failure"},
    {0x4B, "SBC failure"},
    {0x4C, "SBC failure"},
    {0x4D, "SBC failure"},
    {0x4E, "SBC failure"},
    {0x4F, "SBC failure"},
    {0x50, "STA failure"},
    {0x51, "LDY,X failure"},
    {0x52, "LDY,X failure (didn't page cross)"},
    {0x53, "ORA failure"},
    {0x54, "ORA failure"},
    {0x55, "AND failure"},
    {0x56, "AND failure"},
    {0x57, "EOR failure"},
    {0x58, "EOR failure"},
    {0x59, "ADC failure"},
    {0x5A, "ADC failure"},
    {0x5B, "ADC failure"},
    {0x5C, "ADC failure"},
    {0x5D, "ADC failure"},
    {0x5E, "CMP failure"},
    {0x5F, "CMP failure"},
    {0x60, "CMP failure"},
    {0x61, "CMP failure"},
    {0x62, "CMP failure"},
    {0x63, "CMP failure"},
    {0x64, "CMP failure"},
    {0x65, "SBC failure"},
    {0x66, "SBC failure"},
    {0x67, "SBC failure"},
    {0x68, "SBC failure"},
    {0x69, "SBC failure"},
    {0x6A, "LDA failure"},
    {0x6B, "LDA failure (didn't page cross)"},
    {0x6C, "STA failure"},
    {0x6D, "LSR failure"},
    {0x6E, "LSR failure"},
    {0x6F, "ASL failure"},
    {0x70, "ASL failure"},
    {0x71, "ROR failure"},
    {0x72, "ROR failure"},
    {0x73, "ROL failure"},
    {0x74, "ROL failure"},
    {0x75, "INC failure"},
    {0x76, "INC failure"},
    {0x77, "DEC failure"},
    {0x78, "DEC failure"},
    {0x79, "DEC failure"},
    {0x7A, "LDX,Y failure"},
    {0x7B, "LDX,Y failure"},
    {0x4E, "absolute,X NOPs less than 3 bytes long"},
    {0x4F, "implied NOPs affects regs/flags"},
    {0x50, "ZP,X NOPs less than 2 bytes long"},
    {0x51, "absolute NOP less than 3 bytes long"},
    {0x52, "ZP NOPs less than 2 bytes long"},
    {0x53, "absolute,X NOPs less than 3 bytes long"},
    {0x54, "implied NOPs affects regs/flags"},
    {0x55, "ZP,X NOPs less than 2 bytes long"},
    {0x56, "absolute NOP less than 3 bytes long"},
    {0x57, "ZP NOPs less than 2 bytes long"},
    {0x7C, "LAX (indr,x) failure"},
    {0x7D, "LAX (indr,x) failure"},
    {0x7E, "LAX zeropage failure"},
    {0x7F, "LAX zeropage failure"},
    {0x80, "LAX absolute failure"},
    {0x81, "LAX absolute failure"},
    {0x82, "LAX (indr),y failure"},
    {0x83, "LAX (indr),y failure"},
    {0x84, "LAX zp,y failure"},
    {0x85, "LAX zp,y failure"},
    {0x86, "LAX abs,y failure"},
    {0x87, "LAX abs,y failure"},
    {0x88, "SAX (indr,x) failure"},
    {0x89, "SAX (indr,x) failure"},
    {0x8A, "SAX zeropage failure"},
    {0x8B, "SAX zeropage failure"},
    {0x8C, "SAX absolute failure"},
    {0x8D, "SAX absolute failure"},
    {0x8E, "SAX zp,y failure"},
    {0x8F, "SAX zp,y failure"},
    {0x90, "SBC failure"},
    {0x91, "SBC failure"},
    {0x92, "SBC failure"},
    {0x93, "SBC failure"},
    {0x94, "SBC failure"},
    {0x95, "DCP (indr,x) failure"},
    {0x96, "DCP (indr,x) failure"},
    {0x97, "DCP (indr,x) failure"},
    {0x98, "DCP zeropage failure"},
    {0x99, "DCP zeropage failure"},
    {0x9A, "DCP zeropage failure"},
    {0x9B, "DCP absolute failure"},
    {0x9C, "DCP absolute failure"},
    {0x9D, "DCP absolute failure"},
    {0x9E, "DCP (indr),y failure"},
    {0x9F, "DCP (indr),y failure"},
    {0xA0, "DCP (indr),y failure"},
    {0xA1, "DCP zp,x failure"},
    {0xA2, "DCP zp,x failure"},
    {0xA3, "DCP zp,x failure"},
    {0xA4, "DCP abs,y failure"},
    {0xA5, "DCP abs,y failure"},
    {0xA6, "DCP abs,y failure"},
    {0xA7, "DCP abs,x failure"},
    {0xA8, "DCP abs,x failure"},
    {0xA9, "DCP abs,x failure"},
    {0xAA, "DCP (indr,x) failure"},
    {0xAB, "DCP (indr,x) failure"},
    {0xAC, "DCP (indr,x) failure"},
    {0xAD, "DCP zeropage failure"},
    {0xAE, "DCP zeropage failure"},
    {0xAF, "DCP zeropage failure"},
    {0xB0, "DCP absolute failure"},
    {0xB1, "DCP absolute failure"},
    {0xB2, "DCP absolute failure"},
    {0xB3, "DCP (indr),y failure"},
    {0xB4, "DCP (indr),y failure"},
    {0xB5, "DCP (indr),y failure"},
    {0xB6, "DCP zp,x failure"},
    {0xB7, "DCP zp,x failure"},
    {0xB8, "DCP zp,x failure"},
    {0xB9, "DCP abs,y failure"},
    {0xBA, "DCP abs,y failure"},
    {0xBB, "DCP abs,y failure"},
    {0xBC, "DCP abs,x failure"},
    {0xBD, "DCP abs,x failure"},
    {0xBE, "DCP abs,x failure"},
    {0xBF, "SLO (indr,x) failure"},
    {0xC0, "SLO (indr,x) failure"},
    {0xC1, "SLO (indr,x) failure"},
    {0xC2, "SLO zeropage failure"},
    {0xC3, "SLO zeropage failure"},
    {0xC4, "SLO zeropage failure"},
    {0xC5, "SLO absolute failure"},
    {0xC6, "SLO absolute failure"},
    {0xC7, "SLO absolute failure"},
    {0xC8, "SLO (indr),y failure"},
    {0xC9, "SLO (indr),y failure"},
    {0xCA, "SLO (indr),y failure"},
    {0xCB, "SLO zp,x failure"},
    {0xCC, "SLO zp,x failure"},
    {0xCD, "SLO zp,x failure"},
    {0xCE, "SLO abs,y failure"},
    {0xCF, "SLO abs,y failure"},
    {0xD0, "SLO abs,y failure"},
    {0xD1, "SLO abs,x failure"},
    {0xD2, "SLO abs,x failure"},
    {0xD3, "SLO abs,x failure"},
    {0xD4, "RLA (indr,x) failure"},
    {0xD5, "RLA (indr,x) failure"},
    {0xD6, "RLA (indr,x) failure"},
    {0xD7, "RLA zeropage failure"},
    {0xD8, "RLA zeropage failure"},
    {0xD9, "RLA zeropage failure"},
    {0xDA, "RLA absolute failure"},
    {0xDB, "RLA absolute failure"},
    {0xDC, "RLA absolute failure"},
    {0xDD, "RLA (indr),y failure"},
    {0xDE, "RLA (indr),y failure"},
    {0xDF, "RLA (indr),y failure"},
    {0xE0, "RLA zp,x failure"},
    {0xE1, "RLA zp,x failure"},
    {0xE2, "RLA zp,x failure"},
    {0xE3, "RLA abs,y failure"},
    {0xE4, "RLA abs,y failure"},
    {0xE5, "RLA abs,y failure"},
    {0xE6, "RLA abs,x failure"},
    {0xE7, "RLA abs,x failure"},
    {0xE8, "RLA abs,x failure"},
    {0xE8, "SRE (indr,x) failure"},
    {0xEA, "SRE (indr,x) failure"},
    {0xEB, "SRE (indr,x) failure"},
    {0xEC, "SRE zeropage failure"},
    {0xED, "SRE zeropage failure"},
    {0xEE, "SRE zeropage failure"},
    {0xEF, "SRE absolute failure"},
    {0xF0, "SRE absolute failure"},
    {0xF1, "SRE absolute failure"},
    {0xF2, "SRE (indr),y failure"},
    {0xF3, "SRE (indr),y failure"},
    {0xF4, "SRE (indr),y failure"},
    {0xF5, "SRE zp,x failure"},
    {0xF6, "SRE zp,x failure"},
    {0xF7, "SRE zp,x failure"},
    {0xF8, "SRE abs,y failure"},
    {0xF9, "SRE abs,y failure"},
    {0xFA, "SRE abs,y failure"},
    {0xFB, "SRE abs,x failure"},
    {0xFC, "SRE abs,x failure"},
    {0xFD, "SRE abs,x failure"},
};

struct test_t tests_group3[] = {
    {0x01, "RRA (indr,x) failure"},
    {0x02, "RRA (indr,x) failure"},
    {0x03, "RRA (indr,x) failure"},
    {0x04, "RRA zeropage failure"},
    {0x05, "RRA zeropage failure"},
    {0x06, "RRA zeropage failure"},
    {0x07, "RRA absolute failure"},
    {0x08, "RRA absolute failure"},
    {0x09, "RRA absolute failure"},
    {0x0A, "RRA (indr),y failure"},
    {0x0B, "RRA (indr),y failure"},
    {0x0C, "RRA (indr),y failure"},
    {0x0D, "RRA zp,x failure"},
    {0x0E, "RRA zp,x failure"},
    {0x0F, "RRA zp,x failure"},
    {0x10, "RRA abs,y failure"},
    {0x11, "RRA abs,y failure"},
    {0x12, "RRA abs,y failure"},
    {0x13, "RRA abs,x failure"},
    {0x14, "RRA abs,x failure"},
    {0x15, "RRA abs,x failure"},
};

#define ADDR_ERR_CODE 0x00

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

    init_profiler();

    /*
    nestest is all the proof you need that you shouldn't trust documentation and also
    seemingly proves that no one uses it or bothers to mention how it doesn't actually
    work as stated.

    Results are NOT placed in $02h + $03h, they're placed in $00h

    */

    // init test (wait for error code to get 0'd)
    do {
        cpu_pulse(&cpu);
    } while (mem_read_addr(&mem, ADDR_ERR_CODE));

    u16 pc_last = 0xFFFF;
    u8 status_prev = 0;
    int group = 1;
    while (cpu.pc != pc_last || cpu.tcu != 0) {
        if (cpu.tcu == 0) {
            pc_last = cpu.pc;
        }
        cpu_pulse(&cpu);

        u8 status = mem_read_addr(&mem, ADDR_ERR_CODE);

        if (status != status_prev && status != 0) {
            const char *msg = "Unknown";

            struct test_t *group_arr;
            size_t group_size;
            switch (group) {
                case 1:
                    group_arr = tests_group1;
                    group_size = GROUP1_LEN;
                    break;
                case 2:
                    group_arr = tests_group2;
                    group_size = GROUP2_LEN;
                    break;
                case 3:
                    group_arr = tests_group3;
                    group_size = GROUP3_LEN;
                    break;
            }

            if (group_arr) {
                for (unsigned int i = 0; i < group_size; i++) {
                    if (group_arr[i].err_code == status) {
                        msg = group_arr[i].msg;
                        break;
                    }
                }
            }

            printf("\033[31m"
                "[Failed] Test %02X (G%i) @ $%04X: %s"
                "\033[0;39m\n", status, group, cpu.pc, msg);
        }

        switch (cpu.pc) {
            case 0xC623:
                group = 2;
                break;
            case 0xC652:
                group = 3;
                break;
        }

        status_prev = status;
    }


    end_profiler("nestest.profile.json");

    return 0;
}



void fatal(const char *msg)
{
    printf("\033[31m[Fatal] %s\033[0;39m\n", msg);
    exit(2);
}
