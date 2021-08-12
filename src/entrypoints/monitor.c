#include "stdio.h"
#include "signal.h"
#include "ncurses.h"
#include "../headers/rom.h"
#include "../headers/cpu6502.h"

void int_handle(int sig);
void run_monitor(Cpu6502 *cpu);

int main() {

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
}

void draw(Cpu6502 *cpu);

int WIN_MEM_LINES;
int WIN_MEM_BYTES_PER_LINE;
WINDOW *win_current_mem_block;
void draw_mem_block(MemoryBlock *b, memaddr addr);


typedef int _box_intersects;
const int UP = 1 << 0;
const int LEFT = 1 << 1;
const int RIGHT = 1 << 2;
const int DOWN = 1 << 3;
void box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr);

void run_monitor(Cpu6502 *cpu) {
    initscr();
    // FILE *f = fopen("/dev/tty", "r+");
    // set_term(newterm(NULL, f, f));
    // printf("\rfoobar\n");

    curs_set(0);
    noecho();
    raw();
    start_color();
    use_default_colors();
#define COLOR_ADDRESSED 1
    init_pair(COLOR_ADDRESSED, COLOR_BLUE, -1);

    WIN_MEM_LINES = LINES - 2;
    WIN_MEM_BYTES_PER_LINE = 0x10;
    win_current_mem_block = newwin(WIN_MEM_LINES + 2, (WIN_MEM_BYTES_PER_LINE * 3) - 1 + 6 + 4, 0, 0);

    draw(cpu);

    char ch;
    while (1) {
        draw(cpu);
        ch = getch();
        switch (ch)
        {
        case 0x03:
            raise(SIGINT);
            return;
        }
    }
}

void draw(Cpu6502 *cpu) {
    draw_mem_block(
        cpu_is_read(cpu)
            ? mem_get_read_block(cpu->memmap, cpu->addr_bus)
            : mem_get_write_block(cpu->memmap, cpu->addr_bus),
        cpu->addr_bus);
    wrefresh(stdscr);
}

void draw_mem_block(MemoryBlock *b, memaddr addr) {
    char fmt[7];
    wclear(win_current_mem_block);
    box_draw(win_current_mem_block, 0, 0, 0, 0, 0);
    if (b)
    {
        memaddr addr_start = addr & 0xFF00 - 0x100;
        if (addr_start < b->range_low) addr_start = b->range_low & 0xFFF0;

        int val_start = 0;
        mvwaddstr(win_current_mem_block, 0, 2, b->block_name);
        for (int l = 0; l < WIN_MEM_LINES && addr_start <= b->range_high; l++) {
            snprintf(fmt, 7, "%04X: ", addr_start);
            mvwaddstr(win_current_mem_block, l + 1, 2, fmt);

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
        snprintf(fmt, 6, "$%04x: ", addr);
        mvwaddstr(win_current_mem_block, 0, 2, fmt);
        mvwaddstr(win_current_mem_block, 2, 2, "Unaddressable Memory");
    }



    wrefresh(win_current_mem_block);
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