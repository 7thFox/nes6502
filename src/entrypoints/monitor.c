#include "stdio.h"
#include "signal.h"
#include "ncurses.h"
#include "../headers/log.h"
#include "../headers/rom.h"
#include "../headers/cpu6502.h"
#include "../headers/disasm.h"

void int_handle(int sig);
void run_monitor(Cpu6502 *cpu);
FILE *log_file = 0;

int main() {
    if (!(log_file = fopen("monitor.log", "w+")))
    {
        fprintf(stderr, "Failed to open log file");
        return 1;
    }

    Rom rom;
    if (!rom_load(&rom, "./example/scratch.rom")) {
        fprintf(stderr, "Failed parsing rom file.\n");
        return 1;
    }

    // printf("Rom loaded:\n");
    // printf("\tOffset: %04x\n", rom.map_offset);
    // printf("\tSize: %i\n", rom.rom_size);
    // printf("\tContents:");
    // for (int i = 0; i < rom.rom_size; i++) {
    //     if (i % 16 == 0) printf("\n\t\t");
    //     printf("%02x ", rom.value[i]);
    // }
    // printf("\n");

    MemoryMap mem;
    mem.n_read_blocks = 0;
    mem.n_write_blocks = 0;
    mem_add_rom(&mem, &rom);

    Cpu6502 cpu;
    cpu.memmap = &mem;
    cpu.bit_fields |= 1 << 0;
    cpu_resb(&cpu);
    cpu.addr_bus = rom.map_offset;

    signal(SIGINT, int_handle);
    run_monitor(&cpu);
}

void int_handle(int sig) {
    endwin();
    if (log_file) fclose(log_file);
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
void draw_instructions(MemoryBlock *b, uint16_t pc);
Disassembler *disassembler;

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
    bool read = cpu_is_read(cpu);
    draw_mem_block(
        read
            ? mem_get_read_block(cpu->memmap, cpu->addr_bus)
            : mem_get_write_block(cpu->memmap, cpu->addr_bus),
        cpu->addr_bus,
        read);

    memaddr pc = cpu->pch << 8 | cpu->pcl;
    draw_instructions(mem_get_read_block(cpu->memmap, pc), pc);

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

        memaddr addr_start = addr & 0xFF00 - 0x100;
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

    wattron(win_current_mem_block, COLOR_PAIR(COLOR_FANCY));
    mvwaddstr(win_current_mem_block, 0, WIN_MEM_COLS - 1, read ? " R " : " W ");
    wattroff(win_current_mem_block, COLOR_PAIR(COLOR_FANCY));

    wrefresh(win_current_mem_block);
}

void draw_instructions(MemoryBlock *b, memaddr pc) {
    tracef("draw_instructions\n");
    wclear(win_instructions);
    box_draw(win_instructions, LEFT, 0, 0, 0, 0);

    char addr_buff[8];

    uint16_t alignment_addr = disasm_get_alignment(disassembler, pc, WIN_INST_LINES - 1);
    if (alignment_addr < b->range_low) alignment_addr = b->range_low;
    uint16_t offset = alignment_addr - b->range_low;
    uint8_t *a = b->values + offset;
    Disassembly dis = disasm(disassembler, b->values + offset, b->range_high - alignment_addr, WIN_INST_LINES);
    for (int i = 0; i < dis.count; i++)
    {
        wattron(win_instructions, COLOR_PAIR(COLOR_ADDRESS_LABEL));
        uint16_t addr = dis.offsets[i] + alignment_addr;
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

    wrefresh(win_instructions);
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