
#include "stdio.h"
#include "signal.h"
#include "ncurses.h"
#include "../headers/log.h"
#include "../headers/ram.h"
#include "../headers/rom.h"
#include "../headers/cpu6502.h"
#include "../headers/disasm.h"

#define NES_MODE 1

// const char *ROM_FILE = "./example/scratch.rom";
// const char *ROM_FILE = "./example/klaus2m5_functional_test.rom";
const char *ROM_FILE = "./example/nestest-prg.rom";

void ncurses_cleanup() {
    endwin();
}
void run_monitor(Cpu6502 *cpu);

int main() {
    if (!init_logging("monitor.log")) exit(EXIT_FAILURE);
    tracef("main \n");

    enable_stacktrace();

    MemoryMap mem;
    mem.n_read_blocks = 0;
    mem.n_write_blocks = 0;
    mem._ppu = NULL;

    Rom rom;
    if (!rom_load(&rom, ROM_FILE)) {
        fprintf(stderr, "Failed parsing rom file.\n");
        return 1;
    }
    mem_add_rom(&mem, &rom, "ROM");

#if NES_MODE
    Ram zpg_ram;
    zpg_ram.map_offset = 0x0000;
    zpg_ram.size = 0x100;
    zpg_ram.value = malloc(zpg_ram.size);
    mem_add_ram(&mem, &zpg_ram, "ZPG");

    Ram stack_ram;
    stack_ram.map_offset = 0x0100;
    stack_ram.size = 0x0100;
    stack_ram.value = malloc(stack_ram.size);
    mem_add_ram(&mem, &stack_ram, "STACK");

    Ram ram;
    ram.map_offset = 0x0200;
    ram.size = 0x0600;
    ram.value = malloc(ram.size);
    mem_add_ram(&mem, &ram, "RAM");

    PPURegisters ppu;
    ppu.status = 0x02;// just to make sure I can actually read from here
    mem_add_ppu(&mem, &ppu);
#endif

    Cpu6502 cpu;
    cpu.memmap = &mem;
    cpu.addr_bus = rom.map_offset;
    cpu_resb(&cpu);

    signal(SIGINT, ncurses_cleanup);
    run_monitor(&cpu);
}

void draw(Cpu6502 *cpu);

int WIN_MEM_LINES;
int WIN_MEM_BYTES_PER_LINE;
int WIN_MEM_COLS;
WINDOW *win_current_mem_block;
void draw_mem_block(MemoryBlock *b, memaddr addr, bool read);
int WIN_INST_LINES;
int WIN_INST_COLS;
WINDOW *win_instructions;
void draw_instructions(MemoryBlock *b, u16 pc);
Disassembler *disassembler;
int WIN_REG_LINES;
// #define WIN_REG_BORDER1 12
// #define WIN_REG_BORDER2 24
#define WIN_REG_WIDTH_1_4 9
#define WIN_REG_WIDTH_2_4 18
#define WIN_REG_WIDTH_3_4 27
#define WIN_REG_WIDTH_1_3 12
#define WIN_REG_WIDTH_2_3 24
#define WIN_REG_WIDTH 36
#define WIN_REG_COLS WIN_REG_WIDTH - 3
WINDOW *win_registers;
void draw_registers(Cpu6502 *cpu);

void run_monitor(Cpu6502 *cpu) {
    disassembler = create_disassembler();
    initscr();
    curs_set(0);
    noecho();
    raw();
    start_color();
    use_default_colors();
#define COLOR_ADDRESSED 1
    init_pair(COLOR_ADDRESSED, COLOR_BLUE, -1);
#define COLOR_FANCY 2
    init_pair(COLOR_FANCY, COLOR_RED, -1);
#define COLOR_NAME 3
    init_pair(COLOR_NAME, COLOR_YELLOW, -1);
#define COLOR_ADDRESS_LABEL 4
    init_pair(COLOR_ADDRESS_LABEL, COLOR_GREEN, -1);
#define COLOR_UNIMPORTANT_BYTES 5
    init_pair(COLOR_UNIMPORTANT_BYTES, COLOR_CYAN, -1);

    WIN_MEM_LINES = LINES - 2;
    WIN_MEM_BYTES_PER_LINE = 0x10;
    WIN_MEM_COLS = (WIN_MEM_BYTES_PER_LINE * 3) - 1 + 7;
    win_current_mem_block = newwin(WIN_MEM_LINES + 2, WIN_MEM_COLS + 4, 0, 0);

    WIN_INST_LINES = LINES - 2;
    WIN_INST_COLS = 7 /* "$ffff: " */ + N_MAX_BYTE_SIZE + N_MAX_TEXT_SIZE - 2 /* null terminals in count */;
    win_instructions = newwin(WIN_INST_LINES + 2, WIN_INST_COLS + 4, 0, WIN_MEM_COLS+3);

    WIN_REG_LINES = LINES - 2;
    win_registers = newwin(WIN_REG_LINES + 2, WIN_REG_COLS + 4, 0, WIN_MEM_COLS + WIN_INST_COLS + 6);

    wrefresh(stdscr);
    draw(cpu);

    char ch;
    while (1) {
noredraw:
        ch = getchar();
        switch (ch)
        {
        case 0x03:
            raise(SIGINT);
            return;
        case ' ':
            cpu_pulse(cpu);
            break;
        default:
            goto noredraw;
        }
        draw(cpu);
    }
}

typedef int _box_intersects;
const int UP = 1 << 0;
const int LEFT = 1 << 1;
const int RIGHT = 1 << 2;
const int DOWN = 1 << 3;
void box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr);

void draw(Cpu6502 *cpu) {
    bool read = (cpu->bit_fields & PIN_READ) == PIN_READ;
    draw_mem_block(
        read
            ? mem_get_read_block(cpu->memmap, cpu->addr_bus)
            : mem_get_write_block(cpu->memmap, cpu->addr_bus),
        cpu->addr_bus,
        read);

    draw_instructions(mem_get_read_block(cpu->memmap, cpu->pc), cpu->pc);

    draw_registers(cpu);

    tracef("end draw\n");

    wrefresh(stdscr);
}

void draw_mem_block(MemoryBlock *b, memaddr addr, bool read) {
    char fmt[8];
    wclear(win_current_mem_block);
    box_draw(win_current_mem_block, RIGHT, 0, 0, 0, 0);
    wattron(win_current_mem_block, COLOR_PAIR(COLOR_NAME));
    if (b)
    {
        mvwaddstr(win_current_mem_block, 0, 2, b->block_name);
        wattroff(win_current_mem_block, COLOR_PAIR(COLOR_NAME));

        memaddr addr_start = (addr & 0xFF00) - 0x100;
        if (addr_start < b->range_low) addr_start = b->range_low & 0xFFF0;

        int val_start = 0;
        for (int l = 0; l < WIN_MEM_LINES && addr_start <= b->range_high; l++) {
            wattron(win_current_mem_block, COLOR_PAIR(COLOR_ADDRESS_LABEL));
            sprintf(fmt, "$%04X: ", addr_start);
            mvwaddstr(win_current_mem_block, l + 1, 2, fmt);
            wattroff(win_current_mem_block, COLOR_PAIR(COLOR_ADDRESS_LABEL));

            for (int i = 0; i < WIN_MEM_BYTES_PER_LINE && (addr_start + i) <= b->range_high; i++) {
                if (addr_start + i == addr) {
                    wattron(win_current_mem_block, COLOR_PAIR(COLOR_ADDRESSED));
                }
                snprintf(fmt, 4, "%02X ", b->values[val_start + i]);
                waddstr(win_current_mem_block, fmt);
                wattroff(win_current_mem_block, COLOR_PAIR(COLOR_ADDRESSED));
            }

            addr_start += WIN_MEM_BYTES_PER_LINE;
            val_start += WIN_MEM_BYTES_PER_LINE;
        }
    }
    else
    {
        mvwaddstr(win_current_mem_block, 0, 2, "Unaddressable Memory");
        wattroff(win_current_mem_block, COLOR_PAIR(COLOR_NAME));
    }

    sprintf(fmt, " $%04x ", addr);
    mvwaddstr(win_current_mem_block, 0, WIN_MEM_COLS - 9, fmt);

    wattron(win_current_mem_block, COLOR_PAIR(COLOR_FANCY));
    mvwaddstr(win_current_mem_block, 0, WIN_MEM_COLS - 1, read ? " R " : " W ");
    wattroff(win_current_mem_block, COLOR_PAIR(COLOR_FANCY));

    wrefresh(win_current_mem_block);
}

void draw_instructions(MemoryBlock *b, memaddr pc) {
    tracef("draw_instructions\n");
    wclear(win_instructions);
    box_draw(win_instructions, LEFT | RIGHT, 0, 0, 0, 0);

    if (b)
    {
        char addr_buff[8];

        u16 alignment_addr = disasm_get_alignment(disassembler, pc, WIN_INST_LINES - 1);
        if (alignment_addr < b->range_low) alignment_addr = b->range_low;
        u16 offset = alignment_addr - b->range_low;
        Disassembly dis = disasm(disassembler, b->values + offset, b->range_high - alignment_addr, WIN_INST_LINES);
        for (int i = 0; i < dis.count; i++)
        {
            wattron(win_instructions, COLOR_PAIR(COLOR_ADDRESS_LABEL));
            u16 addr = dis.offsets[i] + alignment_addr;
            sprintf(addr_buff, "$%04x: ", addr);
            mvwaddstr(win_instructions, 1 + i, 2, addr_buff);
            wattroff(win_instructions, COLOR_PAIR(COLOR_ADDRESS_LABEL));

            wattron(win_instructions, COLOR_PAIR(COLOR_UNIMPORTANT_BYTES));
            waddstr(win_instructions, dis.bytes[i]);
            wattroff(win_instructions, COLOR_PAIR(COLOR_UNIMPORTANT_BYTES));

            if (addr == pc) wattron(win_instructions, COLOR_PAIR(COLOR_ADDRESSED));
            waddstr(win_instructions, dis.text[i]);
            wattroff(win_instructions, COLOR_PAIR(COLOR_ADDRESSED));
        }
    }
    else
    {
        mvwaddstr(win_instructions, 1, 2, "Unaddressable Memory");
    }

    wrefresh(win_instructions);
}

void draw_registers(Cpu6502 *cpu) {
    wclear(win_registers);
    box_draw(win_registers, LEFT, 0, 0, 0, 0);

    char buff[64];

    mvwaddch(win_registers, 0, WIN_REG_WIDTH_1_3, ACS_TTEE);
    mvwaddch(win_registers, 0, WIN_REG_WIDTH_2_3, ACS_TTEE);
    mvwaddch(win_registers, 0, WIN_REG_WIDTH, ACS_TTEE);

    const int PC_LINE_INDEX = 1;
    sprintf(buff, "PC: $%04x | IR:  %02X   | TCU: %i    |", cpu->pc, cpu->ir, cpu->tcu);
    mvwaddstr(win_registers, PC_LINE_INDEX, 2, buff);
    mvwaddch( win_registers, PC_LINE_INDEX, WIN_REG_WIDTH_1_3, ACS_VLINE);
    mvwaddch( win_registers, PC_LINE_INDEX, WIN_REG_WIDTH_2_3, ACS_VLINE);
    mvwaddch( win_registers, PC_LINE_INDEX, WIN_REG_WIDTH, ACS_VLINE);

    mvwaddch(win_registers, PC_LINE_INDEX+1, 0, ACS_LTEE);
    mvwhline(win_registers, PC_LINE_INDEX+1, 1, ACS_HLINE, WIN_REG_WIDTH - 1);
    mvwaddch(win_registers, PC_LINE_INDEX+1, WIN_REG_WIDTH_1_3, ACS_PLUS);
    mvwaddch(win_registers, PC_LINE_INDEX+1, WIN_REG_WIDTH_2_3, ACS_PLUS);
    mvwaddch(win_registers, PC_LINE_INDEX+1, WIN_REG_WIDTH, ACS_RTEE);

    const int X_LINE_INDEX = 3;
    sprintf(buff, " X: $%02x   |  Y: $%02x   |  A: $%02x   |", cpu->x, cpu->y, cpu->a);
    mvwaddstr(win_registers, X_LINE_INDEX, 2, buff);
    mvwaddch( win_registers, X_LINE_INDEX, WIN_REG_WIDTH_1_3, ACS_VLINE);
    mvwaddch( win_registers, X_LINE_INDEX, WIN_REG_WIDTH_2_3, ACS_VLINE);
    mvwaddch( win_registers, X_LINE_INDEX, WIN_REG_WIDTH, ACS_VLINE);

    mvwaddch(win_registers, X_LINE_INDEX + 1, 0, ACS_LTEE);
    mvwhline(win_registers, X_LINE_INDEX + 1, 1, ACS_HLINE, WIN_REG_WIDTH - 1);
    mvwaddch(win_registers, X_LINE_INDEX + 1, WIN_REG_WIDTH_1_3, ACS_PLUS);
    mvwaddch(win_registers, X_LINE_INDEX + 1, WIN_REG_WIDTH_2_3, ACS_PLUS);
    mvwaddch(win_registers, X_LINE_INDEX + 1, WIN_REG_WIDTH, ACS_RTEE);

    const int SP_LINE_INDEX = 5;
    sprintf(buff, "SP: $%02x   | PD: $%02x   |           |", cpu->sp, cpu->pd);
    mvwaddstr(win_registers, SP_LINE_INDEX, 2, buff);
    mvwaddch( win_registers, SP_LINE_INDEX, WIN_REG_WIDTH_1_3, ACS_VLINE);
    mvwaddch( win_registers, SP_LINE_INDEX, WIN_REG_WIDTH_2_3, ACS_VLINE);
    mvwaddch( win_registers, SP_LINE_INDEX, WIN_REG_WIDTH, ACS_VLINE);

    mvwaddch(win_registers, SP_LINE_INDEX+1, 0, ACS_LTEE);
    mvwhline(win_registers, SP_LINE_INDEX+1, 1, ACS_HLINE, WIN_REG_WIDTH - 1);
    mvwaddch(win_registers, SP_LINE_INDEX+1, WIN_REG_WIDTH_1_3, ACS_BTEE);
    mvwaddch(win_registers, SP_LINE_INDEX+1, WIN_REG_WIDTH_2_3, ACS_BTEE);
    mvwaddch(win_registers, SP_LINE_INDEX+1, WIN_REG_WIDTH, ACS_RTEE);
    mvwaddch(win_registers, SP_LINE_INDEX+1, WIN_REG_WIDTH_1_4, ACS_TTEE);
    mvwaddch(win_registers, SP_LINE_INDEX+1, WIN_REG_WIDTH_2_4, ACS_TTEE);
    mvwaddch(win_registers, SP_LINE_INDEX+1, WIN_REG_WIDTH_3_4, ACS_TTEE);

    const int P_LINE_INDEX = 7;
    sprintf(buff, " N: %i  |  V: %i  |  _: %i  |  B: %i",
        (cpu->p >> 7) & 0b1,
        (cpu->p >> 6) & 0b1,
        (cpu->p >> 5) & 0b1,
        (cpu->p >> 4) & 0b1);
    mvwaddstr(win_registers, P_LINE_INDEX, 2, buff);
    mvwaddch( win_registers, P_LINE_INDEX, WIN_REG_WIDTH_1_4, ACS_VLINE);
    mvwaddch( win_registers, P_LINE_INDEX, WIN_REG_WIDTH_2_4, ACS_VLINE);
    mvwaddch( win_registers, P_LINE_INDEX, WIN_REG_WIDTH_3_4, ACS_VLINE);
    mvwaddch( win_registers, P_LINE_INDEX, WIN_REG_WIDTH, ACS_VLINE);

    mvwaddch(win_registers, P_LINE_INDEX+1, 0, ACS_LTEE);
    mvwhline(win_registers, P_LINE_INDEX+1, 1, ACS_HLINE, WIN_REG_WIDTH - 1);
    mvwaddch(win_registers, P_LINE_INDEX+1, WIN_REG_WIDTH_1_4, ACS_PLUS);
    mvwaddch(win_registers, P_LINE_INDEX+1, WIN_REG_WIDTH_2_4, ACS_PLUS);
    mvwaddch(win_registers, P_LINE_INDEX+1, WIN_REG_WIDTH_3_4, ACS_PLUS);
    mvwaddch(win_registers, P_LINE_INDEX+1, WIN_REG_WIDTH, ACS_RTEE);

    sprintf(buff, " D: %i  |  I: %i  |  Z: %i  |  C: %i",
        (cpu->p >> 3) & 0b1,
        (cpu->p >> 2) & 0b1,
        (cpu->p >> 1) & 0b1,
        (cpu->p >> 0) & 0b1);
    mvwaddstr(win_registers, P_LINE_INDEX+2, 2, buff);
    mvwaddch( win_registers, P_LINE_INDEX+2, WIN_REG_WIDTH_1_4, ACS_VLINE);
    mvwaddch( win_registers, P_LINE_INDEX+2, WIN_REG_WIDTH_2_4, ACS_VLINE);
    mvwaddch( win_registers, P_LINE_INDEX+2, WIN_REG_WIDTH_3_4, ACS_VLINE);
    mvwaddch( win_registers, P_LINE_INDEX+2, WIN_REG_WIDTH, ACS_VLINE);

    mvwaddch(win_registers, P_LINE_INDEX+3, 0, ACS_LTEE);
    mvwhline(win_registers, P_LINE_INDEX+3, 1, ACS_HLINE, WIN_REG_WIDTH - 1);
    mvwaddch(win_registers, P_LINE_INDEX+3, WIN_REG_WIDTH_1_4, ACS_BTEE);
    mvwaddch(win_registers, P_LINE_INDEX+3, WIN_REG_WIDTH_2_4, ACS_BTEE);
    mvwaddch(win_registers, P_LINE_INDEX+3, WIN_REG_WIDTH_3_4, ACS_BTEE);
    mvwaddch(win_registers, P_LINE_INDEX+3, WIN_REG_WIDTH, ACS_RTEE);

    wrefresh(win_registers);
}

void box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr)
{
    if (ul == 0)
        ul = ((intersect & UP) == UP) && ((intersect & LEFT) == LEFT) ? ACS_PLUS : ((intersect & UP) == UP) ? ACS_LTEE : ((intersect & LEFT) == LEFT) ? ACS_TTEE : ACS_ULCORNER;

    if (ur == 0)
        ur = ((intersect & UP) == UP) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS : ((intersect & UP) == UP) ? ACS_RTEE : ((intersect & RIGHT) == RIGHT) ? ACS_TTEE : ACS_URCORNER;

    if (ll == 0)
        ll = ((intersect & DOWN) == DOWN) && ((intersect & LEFT) == LEFT) ? ACS_PLUS : ((intersect & DOWN) == DOWN) ? ACS_LTEE : ((intersect & LEFT) == LEFT) ? ACS_BTEE : ACS_LLCORNER;

    if (lr == 0)
        lr = ((intersect & DOWN) == DOWN) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS : ((intersect & DOWN) == DOWN) ? ACS_RTEE : ((intersect & RIGHT) == RIGHT) ? ACS_BTEE : ACS_LRCORNER;

    wborder(win,
            ACS_VLINE,
            ACS_VLINE,
            ACS_HLINE,
            ACS_HLINE,
            ul,
            ur,
            ll,
            lr);
}