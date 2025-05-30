
#include "../headers/cpu6502.h"
#include "../headers/disasm.h"
#include "../headers/log.h"
#include "../headers/ram.h"
#include "../headers/rom.h"
#include "../headers/profile.h"

#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#ifdef _WIN32
#include <SDL/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "unistd.h"
#include "execinfo.h"
#include "signal.h"
#include "stdio.h"
#include "time.h"

#define FONT_SIZE 12

#define RIGHT_PADDING_FIX 2
#define BOTTOM_PADDING_FIX 1

#define DEBUG_START 0 // normal resb logic
// #define DEBUG_START 0xCEEE

// const char *ROM_FILE = "./example/scratch.rom";
// const char *ROM_FILE = "./example/klaus2m5_functional_test.rom";
const char *ROM_FILE = "./example/nestest-prg.rom";

// void cleanup() {
//     endwin();

//     end_profiler("monitor.sim.profile.json");
// }

struct simulation_t
{
    Cpu6502      cpu;
    MemoryMap    mem;
    PPURegisters ppu;
    Ram          ram;
    Rom          rom;
    Ram          stack_ram;
    Ram          zpg_ram;
};

struct rendering_t
{
    SDL_Window   *main_win;
    SDL_Renderer *main_rend;

    TTF_Font *font;
    int       font_w;
    int       font_h;
};

struct simstate_t
{
    bool exit;
};

struct monitor_t
{
    struct simulation_t sim;
    struct rendering_t  rend;
    struct simstate_t   state;
};

struct boxbound_t
{
    int x_margin;  int y_margin;
    int x_padding; int y_padding;
};

void user_input(struct simstate_t *s);
void run_sim(struct simstate_t state, struct simulation_t *sim);

SDL_Rect clamp(SDL_Rect rect, int w, int h);
void render_text_box(
    SDL_Surface *dest,
    SDL_Surface *text,
    SDL_Rect rect,
    Uint32 rgb,
    struct boxbound_t tbox);

void render(struct monitor_t *m);

#define subrender(name) SDL_Surface* render_##name(struct monitor_t *m, int w_totmax, int h_totmax)

subrender(stack);
subrender(ram);
subrender(rom);
subrender(inst);
subrender(cpu);
subrender(ppu);

SDL_Color fg_default;
SDL_Color bg_default;

int main() {
    int exit_code = 1;

    // Debugging setup
    {
        if (!init_logging("monitor.log"))
            exit(EXIT_FAILURE);

        init_profiler();
        tracef("main \n");

        enable_stacktrace();
    }

    struct monitor_t monitor;

    // Setup SDL
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS))
        {
            fprintf(stderr, "Failed to init SDL\n");
            goto cleanup;
        }

        if (TTF_Init()) {
            fprintf(stderr, "Failed to init TTF\n");
            goto cleanup;
        }


        monitor.rend.main_win = SDL_CreateWindow(
            "nterm",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            1920, 1080,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
        if (!monitor.rend.main_win) {
            fprintf(stderr, "Failed to create window\n");
            goto cleanup;
        }

        monitor.rend.main_rend = SDL_CreateRenderer(
            monitor.rend.main_win,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!monitor.rend.main_rend){
            fprintf(stderr, "Failed to create renderer\n");
            goto cleanup;
        }

        // TODO JOSH:
        //  fc-match --format=%{file} DroidSansMono.ttf
        monitor.rend.font = TTF_OpenFont("/usr/share/fonts/noto/NotoSansMono-Regular.ttf", FONT_SIZE);
        if (!monitor.rend.font) {
            fprintf(stderr, "Failed to open font: %s\n", SDL_GetError());
            goto cleanup;
        }

        TTF_SetFontHinting(monitor.rend.font, TTF_HINTING_MONO);


        SDL_Surface *test_surface = TTF_RenderText_Shaded(monitor.rend.font, "M",
            (SDL_Color){ 0xFF, 0xFF, 0xFF, 0xFF },
            (SDL_Color){ 0x00, 0x00, 0x00, 0xFF });
        monitor.rend.font_h = test_surface->h;
        monitor.rend.font_w = test_surface->w;
        SDL_FreeSurface(test_surface);
    }

    // Setup Colors
    {
        fg_default = (SDL_Color){ 0xFF, 0xFF, 0xFF, 0xFF, };
        bg_default = (SDL_Color){ 0x00, 0x00, 0x00, 0x00, };
    }

    // Setup Cpu
    {
        monitor.sim.mem.n_read_blocks  = 0;
        monitor.sim.mem.n_write_blocks = 0;
        monitor.sim.mem._ppu           = NULL;

        if (!rom_load(&monitor.sim.rom, ROM_FILE)) {
            fprintf(stderr, "Failed parsing rom file.\n");
            return 1;
        }
        mem_add_rom(&monitor.sim.mem, &monitor.sim.rom, "ROM");

        monitor.sim.zpg_ram.map_offset = 0x0000;
        monitor.sim.zpg_ram.size       = 0x100;
        monitor.sim.zpg_ram.value      = malloc(monitor.sim.zpg_ram.size);
        mem_add_ram(&monitor.sim.mem, &monitor.sim.zpg_ram, "ZPG");

        monitor.sim.stack_ram.map_offset = 0x0100;
        monitor.sim.stack_ram.size       = 0x0100;
        monitor.sim.stack_ram.value      = malloc(monitor.sim.stack_ram.size);
        mem_add_ram(&monitor.sim.mem, &monitor.sim.stack_ram, "STACK");

        monitor.sim.ram.map_offset = 0x0200;
        monitor.sim.ram.size       = 0x0600;
        monitor.sim.ram.value      = malloc(monitor.sim.ram.size);
        mem_add_ram(&monitor.sim.mem, &monitor.sim.ram, "RAM");

        monitor.sim.ppu.status = 0xA2; // just to make sure I can actually read from here
        mem_add_ppu(&monitor.sim.mem, &monitor.sim.ppu);

        monitor.sim.cpu.memmap   = &monitor.sim.mem;
        monitor.sim.cpu.addr_bus = monitor.sim.rom.map_offset;
        cpu_resb(&monitor.sim.cpu);

        if (DEBUG_START) {
            monitor.sim.cpu.pc = DEBUG_START;
            monitor.sim.cpu.addr_bus = DEBUG_START;
        }
    }

    monitor.state.exit = false;

    while (!monitor.state.exit)
    {
        user_input(&monitor.state);
        run_sim(monitor.state, &monitor.sim);
        render(&monitor);
    }

    exit_code = 0;

cleanup:
    if (monitor.rend.font) TTF_CloseFont(monitor.rend.font);
    if (monitor.rend.main_rend) SDL_DestroyRenderer(monitor.rend.main_rend);
    if (monitor.rend.main_win) SDL_DestroyWindow(monitor.rend.main_win);
    // IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return exit_code;

    // signal(SIGINT, cleanup);
    // run_monitor(&cpu);
    // ncurses_cleanup();
}

void user_input(struct simstate_t *s)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                s->exit = true;
                break;
            case SDL_WINDOWEVENT:
                switch (ev.window.event) {
                    // case SDL_WINDOWEVENT_RESIZED:
                    // case SDL_WINDOWEVENT_MAXIMIZED:
                    //     // do render
                    //     break;
                    case SDL_WINDOWEVENT_CLOSE:
                        s->exit = true;
                        break;
                }
                break;
            // case SDL_KEYDOWN:
            //     switch (ev.key.keysym.sym)
            //     {
            //         case SDLK_BACKSPACE:
            //             if (term->user_input_pos > 0)
            //             {
            //                 term->user_input_buf[term->user_input_pos-1] = '\0';
            //                 term->user_input_pos--;
            //                 term->do_render_clear = true;
            //             }
            //             break;
            //     }
            //     break;
            // case SDL_TEXTINPUT:
            //     for (int i = 0; ev.text.text[i] != '\0'; i++)
            //     {
            //         if (term->user_input_pos < USER_INPUT_BUFFER_MAX)
            //         {
            //             term->user_input_buf[term->user_input_pos] = ev.text.text[i];
            //             term->user_input_buf[term->user_input_pos+1] = '\0';
            //             term->user_input_pos++;
            //         }
            //     }
            //     break;
        }
    }
}

void run_sim(struct simstate_t state, struct simulation_t *sim)
{

}

void render(struct monitor_t *m)
{
    SDL_RenderClear(m->rend.main_rend);

    int w, h;
    SDL_GetWindowSize(m->rend.main_win, &w, &h);

    // Create Surfaces
    SDL_Surface *s_cpu = render_cpu(m, w, h);

    // Create Textures
    SDL_Texture *t_cpu = SDL_CreateTextureFromSurface(m->rend.main_rend, s_cpu);

    // Arrange Surfaces and RenderCopy
    SDL_RenderCopy(m->rend.main_rend, t_cpu, NULL, &(SDL_Rect){
        w-s_cpu->w, 0,
        s_cpu->w,   s_cpu->h});

    // destory surfaces and textures
    SDL_FreeSurface(s_cpu);
    SDL_DestroyTexture(t_cpu);

    SDL_RenderPresent(m->rend.main_rend);
}


subrender(stack)
{
    return NULL;
}

subrender(ram)
{
    return NULL;
}

subrender(rom)
{
    return NULL;
}

subrender(inst)
{
    return NULL;
}

subrender(cpu)
{
    const int nbuff = 32;
    char buff[nbuff];

    // Create componants

    // PC, IR, TCU
    // X, Y, A
    // SP, PD, CYC
    // N V _ B D I Z C

    int w1 = m->rend.font_w * 3;
    SDL_Surface *header = TTF_RenderText_Blended(m->rend.font, "CPU", fg_default);

    int w2 = m->rend.font_w * 9;
    snprintf(buff, nbuff, "PC: $%04x", m->sim.cpu.pc);// 9
    SDL_Surface *pc = TTF_RenderText_Blended(m->rend.font, buff, fg_default);
    snprintf(buff, nbuff, "IR: %02X", m->sim.cpu.ir);// 6
    SDL_Surface *ir = TTF_RenderText_Blended(m->rend.font, buff, fg_default);
    snprintf(buff, nbuff, "TCU: %i", m->sim.cpu.tcu);// 9
    SDL_Surface *tcu = TTF_RenderText_Blended(m->rend.font, buff, fg_default);

    SDL_Surface *boxes[] = {
        header,         NULL,
        pc, ir, tcu,    NULL,
    };

    int nboxes = sizeof(boxes) / sizeof(SDL_Surface*);

    // Calculate layout

    struct boxbound_t bound = {
        .x_margin  = 4, .y_margin  = 4,
        .x_padding = 8, .y_padding = 2 };

    int wmax = 0;
    int hmax = 0;
    int nlines = 0;
    for (int i = 0; i < nboxes; i++)
    {
        nlines++;

        int wline = 0;
        int hline = 0;
        int ncols = 0;
        for (; boxes[i]; i++)
        {
            ncols++;
            if (boxes[i]->w > wline) wline = boxes[i]->w;
            if (boxes[i]->h > hline) hline = boxes[i]->h;
        }

        wline += RIGHT_PADDING_FIX;
        hline += BOTTOM_PADDING_FIX;

        int wsum = ncols     * wline
                 + ncols*2   * bound.x_padding
                 + (ncols+1) * bound.x_margin;

        if (wsum > wmax) wmax = wsum;
        if (hline > hmax) hmax = hline;
    }

    int h = nlines     * hmax
          + nlines*2   * bound.y_padding
          + (nlines+1) * bound.y_margin;

    SDL_Surface *final = SDL_CreateRGBSurface(
        0,
        wmax,
        h,
        32,
        0x00, 0x00, 0x00, 0x00);

    // Uint32 rgb = SDL_MapRGB(final->format, 0x00, 0x00, 0xFF);
    Uint32 rgb = SDL_MapRGB(final->format, 0xFF, 0x00, 0x00);

    int y_offset = 0;
    for (int i = 0; i < nboxes; i++)
    {
        int wline = 0;
        int ncols;
        for (ncols = 0; boxes[i+ncols]; ncols++)
        {
            if (boxes[i]->w > wline) wline = boxes[i]->w;
        }

        int wsum = ncols     * wline
                 + ncols*2   * bound.x_padding
                 + (ncols+1) * bound.x_margin;

        int extra = wmax - wsum;

        wline += extra / ncols;

        int x_offset = 0;
        for (;boxes[i]; i++)
        {
            render_text_box(final,
                boxes[i],
                (SDL_Rect){
                    x_offset,
                    y_offset,
                    wline + 2*bound.x_padding + 2*bound.x_margin,
                    hmax  + 2*bound.y_padding + 2*bound.y_margin},
                rgb,
                bound);

            x_offset += wline + 2*bound.x_padding + bound.x_margin;
        }
        y_offset += hmax + 2*bound.y_padding + bound.y_margin;
    }

    // render_text_box(final, header, r_header, tbox);
    // render_text_box(final, pc,     r_pc,     tbox);
    // render_text_box(final, ir,     r_ir,     tbox);
    // render_text_box(final, tcu,    r_tcu,    tbox);

    // Free componant surfaces

    for (int i = 0; i < nboxes; i++)
    {
        if (!boxes[i]) continue;
        SDL_FreeSurface(boxes[i]);
    }

    return final;
}

subrender(ppu)
{
    return NULL;
}


SDL_Rect clamp(SDL_Rect rect, int w, int h)
{
    if (rect.w > w) rect.w = w;
    if (rect.h > h) rect.h = h;
    return rect;
}

void render_text_box(
    SDL_Surface *dest,
    SDL_Surface *text,
    SDL_Rect rect,
    Uint32 rgb,
    struct boxbound_t bound)
{
    int x_tot = 2*(bound.x_margin + bound.x_padding);
    int y_tot = 2*(bound.y_margin + bound.y_padding);

    SDL_FillRect(dest,
        &(SDL_Rect){
            rect.x + bound.x_margin,
            rect.y + bound.y_margin,
            rect.w - 2*bound.x_margin,
            rect.h - 2*bound.y_margin},
        rgb);
    SDL_BlitSurface(
        text, NULL,
        dest, &(SDL_Rect){
            rect.x + bound.x_margin + bound.x_padding,
            rect.y + bound.y_margin + bound.y_padding,
            rect.w - x_tot,
            rect.h - y_tot });
}

// void draw(Cpu6502 *cpu);

// bool run_until_addr = false;
// u16  addr_to_stop;
// bool run_to_addr();

// // int           WIN_MEM_LINES;
// // int           WIN_STACK_LINES;
// // int           WIN_MEM_BYTES_PER_LINE;
// // int           WIN_MEM_COLS;
// // WINDOW *      win_current_mem_block;
// // WINDOW *      win_stack_mem_block;
// void          draw_mem_block(WINDOW *win, int num_lines, MemoryBlock *b, memaddr addr, bool read, u16 leading);
// // int           WIN_INST_LINES;
// // int           WIN_INST_COLS;
// // WINDOW *      win_instructions;
// void          draw_instructions(MemoryBlock *b, u16 pc);
// Disassembler *disassembler;

// #define COL_REG_WIDTH_1_4 9
// #define COL_REG_WIDTH_2_4 18
// #define COL_REG_WIDTH_3_4 27
// #define COL_REG_WIDTH_1_3 12
// #define COL_REG_WIDTH_2_3 24
// #define COL_REG_WIDTH     36
// #define COL_REG_COLS      COL_REG_WIDTH - 3
// int     WIN_REG_LINES;
// WINDOW *win_registers;
// void    draw_cpu_registers(Cpu6502 *cpu);
// int     WIN_PPU_LINES;
// WINDOW *win_ppu_registers;
// void    draw_ppu_registers(PPURegisters *ppu);

// void run_monitor(Cpu6502 *cpu) {
//     disassembler = create_disassembler();
//     initscr();
//     curs_set(0);
//     noecho();
//     raw();
//     start_color();
//     use_default_colors();
// #define COLOR_ADDRESSED 1
//     init_pair(COLOR_ADDRESSED, COLOR_BLUE, -1);
// #define COLOR_FANCY 2
//     init_pair(COLOR_FANCY, COLOR_RED, -1);
// #define COLOR_NAME 3
//     init_pair(COLOR_NAME, COLOR_YELLOW, -1);
// #define COLOR_ADDRESS_LABEL 4
//     init_pair(COLOR_ADDRESS_LABEL, COLOR_GREEN, -1);
// #define COLOR_UNIMPORTANT_BYTES 5
//     init_pair(COLOR_UNIMPORTANT_BYTES, COLOR_CYAN, -1);

//     WIN_MEM_BYTES_PER_LINE = 0x10;
//     WIN_MEM_COLS           = (WIN_MEM_BYTES_PER_LINE * 3) - 1 + 7;
//     WIN_STACK_LINES        = 0x100 / WIN_MEM_BYTES_PER_LINE + 1;
//     win_stack_mem_block    = newwin(WIN_STACK_LINES + 2, WIN_MEM_COLS + 4, 0, 0);
//     WIN_MEM_LINES          = LINES - WIN_STACK_LINES - 2;
//     win_current_mem_block  = newwin(WIN_MEM_LINES + 2, WIN_MEM_COLS + 4, WIN_STACK_LINES, 0);

//     WIN_INST_LINES   = LINES - 2;
//     WIN_INST_COLS    = 7 /* "$ffff: " */ + N_MAX_BYTE_SIZE + N_MAX_TEXT_SIZE - 2 /* null terminals in count */;
//     win_instructions = newwin(WIN_INST_LINES + 2, WIN_INST_COLS + 4, 0, WIN_MEM_COLS + 3);

//     WIN_REG_LINES = 11;
//     win_registers = newwin(WIN_REG_LINES + 2, COL_REG_COLS + 4, 0, WIN_MEM_COLS + WIN_INST_COLS + 6);

//     WIN_PPU_LINES     = LINES - WIN_REG_LINES - 1;
//     win_ppu_registers = newwin(WIN_PPU_LINES + 2, COL_REG_COLS + 4, WIN_REG_LINES - 1, WIN_MEM_COLS + WIN_INST_COLS + 6);

//     wrefresh(stdscr);
//     draw(cpu);

//     char ch;
//     while (1) {
//     noredraw:

//         if (run_until_addr) {
//             if (addr_to_stop == cpu->pc && cpu->tcu == 0) {
//                 run_until_addr = false;
//                 timeout(-1);
//                 goto noredraw;
//             }

//             // timeout(cpu->pc == lastpc ? 0 : 250);
//             timeout(0);

//             ch = getch();
//             if (ch == 0x03) {
//                 raise(SIGINT);
//                 return;
//             }
//             if (ch == 0x1b) // ESC
//             {
//                 run_until_addr = false;
//                 timeout(-1);
//                 goto noredraw;
//             }

//             cpu_pulse(cpu);
//             if (cpu->tcu == 0) {
//                 draw(cpu);
//             }
//         }
//         else {
//             ch = getchar();
//             switch (ch) {
//             case 0x03:
//                 raise(SIGINT);
//                 return;
//             case ' ':
//                 cpu_pulse(cpu);
//                 break;
//             case 'r':
//                 if (!run_to_addr()) {
//                     raise(SIGINT);
//                     return;
//                 }
//                 break;
//             case 'q':
//                 return;
//             default:
//                 goto noredraw;
//             }
//             draw(cpu);
//         }
//     }
// }

// bool run_to_addr() {
//     wrefresh(stdscr);
//     WINDOW *prompt = newwin(3, 32, 10, 10);
//     box(prompt, 0, 0);

//     mvwaddstr(prompt, 2, 2, " ESC ");
//     mvwaddstr(prompt, 2, 23, " ENTER ");
//     mvwaddstr(prompt, 1, 2, "Run until PC: $");
//     const int startloc = 17;

//     addr_to_stop = 0;

//     int i = 0;
//     while (1) {
//         wrefresh(prompt);
//         char c = getchar();
//         switch (c) {
//         case 0x1B: // ESC
//             return true;
//         case 0x03:
//             raise(SIGINT);
//             return false;
//         case 0x7F:
//             i--;
//             addr_to_stop >>= 4;
//             mvwaddch(prompt, 1, startloc + i, ' ');
//             wmove(prompt, 1, startloc + i);
//             break;
//         case '\r':
//         case '\n':
//             run_until_addr = i > 0;
//             return true;
//         default:
//             if (i < 4) {
//                 if (c >= '0' && c <= '9') {
//                     i++;
//                     addr_to_stop <<= 4;
//                     addr_to_stop += c - '0';
//                     waddch(prompt, c);
//                 }
//                 else if (c >= 'A' && c <= 'F') {
//                     i++;
//                     addr_to_stop <<= 4;
//                     addr_to_stop += c - 'A' + 0xA;
//                     waddch(prompt, c);
//                 }
//                 else if (c >= 'a' && c <= 'f') {
//                     i++;
//                     addr_to_stop <<= 4;
//                     addr_to_stop += c - 'a' + 0xA;
//                     waddch(prompt, c);
//                 }
//             }
//             break;
//         }
//     }
// }

// typedef int _box_intersects;
// const int   UP    = 1 << 0;
// const int   LEFT  = 1 << 1;
// const int   RIGHT = 1 << 2;
// const int   DOWN  = 1 << 3;
// void        box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr);

// void draw(Cpu6502 *cpu) {
//     bool read = (cpu->bit_fields & PIN_READ) == PIN_READ;
//     draw_mem_block(win_stack_mem_block, WIN_STACK_LINES,
//         mem_get_read_block(cpu->memmap, 0x0100),
//         cpu->addr_bus,
//         read,
//         0x100);

//     draw_mem_block(win_current_mem_block, WIN_MEM_LINES,
//         read
//             ? mem_get_read_block(cpu->memmap, cpu->addr_bus)
//             : mem_get_write_block(cpu->memmap, cpu->addr_bus),
//         cpu->addr_bus,
//         read,
//         (cpu->addr_bus & 0xFF00) - 0x100);

//     draw_instructions(mem_get_read_block(cpu->memmap, cpu->pc), cpu->pc);

//     draw_cpu_registers(cpu);
//     draw_ppu_registers(cpu->memmap->_ppu);

//     tracef("end draw\n");

//     wrefresh(stdscr);
// }

// void draw_mem_block(WINDOW *win, int num_lines, MemoryBlock *b, memaddr addr, bool read, u16 addr_start) {
//     char fmt[8];
//     wclear(win);
//     box_draw(win, RIGHT, 0, 0, 0, 0);
//     wattron(win, COLOR_PAIR(COLOR_NAME));
//     if (b) {
//         mvwaddstr(win, 0, 2, b->block_name);
//         wattroff(win, COLOR_PAIR(COLOR_NAME));

//         for (int l = 0; l < num_lines; l++) {
//             wattron(win, COLOR_PAIR(COLOR_ADDRESS_LABEL));
//             sprintf(fmt, "$%04X: ", (addr_start + (l * WIN_MEM_BYTES_PER_LINE)) % 0x10000);
//             mvwaddstr(win, l + 1, 2, fmt);
//             wattroff(win, COLOR_PAIR(COLOR_ADDRESS_LABEL));

//             for (int i = 0; i < WIN_MEM_BYTES_PER_LINE; i++) {
//                 u16 myaddr = addr_start + (l * WIN_MEM_BYTES_PER_LINE) + i;
//                 if (myaddr >= b->range_low && myaddr <= b->range_high) {
//                     if (myaddr == addr) {
//                         wattron(win, COLOR_PAIR(COLOR_ADDRESSED));
//                     }
//                     snprintf(fmt, 4, "%02X ", b->values[myaddr - b->range_low]);
//                     waddstr(win, fmt);
//                     wattroff(win, COLOR_PAIR(COLOR_ADDRESSED));
//                 }
//             }
//         }
//     }
//     else {
//         mvwaddstr(win, 0, 2, "Unaddressable Memory");
//         wattroff(win, COLOR_PAIR(COLOR_NAME));
//     }

//     if (b == NULL || (b->range_low <= addr && b->range_high >= addr)) {
//         sprintf(fmt, " $%04x ", addr);
//         mvwaddstr(win, 0, WIN_MEM_COLS - 9, fmt);

//         wattron(win, COLOR_PAIR(COLOR_FANCY));
//         mvwaddstr(win, 0, WIN_MEM_COLS - 1, read ? " R " : " W ");
//         wattroff(win, COLOR_PAIR(COLOR_FANCY));
//     }

//     wrefresh(win);
// }

// void draw_instructions(MemoryBlock *b, memaddr pc) {
//     tracef("draw_instructions\n");
//     wclear(win_instructions);
//     box_draw(win_instructions, LEFT | RIGHT, 0, 0, 0, 0);

//     if (b) {
//         char addr_buff[8];

//         u16 alignment_addr = disasm_get_alignment(disassembler, pc, WIN_INST_LINES - 1);
//         if (alignment_addr < b->range_low) {
//             alignment_addr = b->range_low;
//         }

//         u16         offset = alignment_addr - b->range_low;
//         Disassembly dis    = disasm(disassembler, b->values + offset, b->range_high - alignment_addr, WIN_INST_LINES);
//         for (int i = 0; i < dis.countInst; i++) {
//             wattron(win_instructions, COLOR_PAIR(COLOR_ADDRESS_LABEL));
//             u16 addr = dis.offsets[i] + alignment_addr;
//             sprintf(addr_buff, "$%04x: ", addr);
//             mvwaddstr(win_instructions, 1 + i, 2, addr_buff);
//             wattroff(win_instructions, COLOR_PAIR(COLOR_ADDRESS_LABEL));

//             wattron(win_instructions, COLOR_PAIR(COLOR_UNIMPORTANT_BYTES));
//             waddstr(win_instructions, dis.bytes[i]);
//             wattroff(win_instructions, COLOR_PAIR(COLOR_UNIMPORTANT_BYTES));

//             if (addr == pc) {
//                 wattron(win_instructions, COLOR_PAIR(COLOR_ADDRESSED));
//             }
//             waddstr(win_instructions, dis.text[i]);
//             wattroff(win_instructions, COLOR_PAIR(COLOR_ADDRESSED));
//         }
//     }
//     else {
//         mvwaddstr(win_instructions, 1, 2, "Unaddressable Memory");
//     }

//     wrefresh(win_instructions);
// }

// void draw_cpu_registers(Cpu6502 *cpu) {
//     wclear(win_registers);
//     box_draw(win_registers, LEFT, 0, 0, 0, 0);

//     char buff[64];

//     mvwaddch(win_registers, 0, 0, ACS_TTEE);
//     mvwaddch(win_registers, 0, COL_REG_WIDTH_1_3, ACS_TTEE);
//     mvwaddch(win_registers, 0, COL_REG_WIDTH_2_3, ACS_TTEE);
//     mvwaddch(win_registers, 0, COL_REG_WIDTH, ACS_URCORNER);

//     wattron(win_registers, COLOR_PAIR(COLOR_NAME));
//     mvwaddstr(win_registers, 0, 2, "CPU");
//     wattroff(win_registers, COLOR_PAIR(COLOR_NAME));

//     void * bt[1] = {(void *)cpu->on_next_clock};
//     char **b     = backtrace_symbols(bt, 1);
//     if (b) {
//         int start = 0;
//         while (b[0][start] != '(') start++;

//         int end = start;
//         while (b[0][end] != '+') end++;

//         int len = end - start - 1;
//         sprintf(buff, " %.*s ", len, b[0] + start + 1);
//         wattron(win_registers, COLOR_PAIR(COLOR_NAME));
//         mvwaddstr(win_registers, 0, COL_REG_WIDTH - len - 2, buff);
//         wattroff(win_registers, COLOR_PAIR(COLOR_NAME));
//         free(b);
//     }

//     const int PC_LINE_INDEX = 1;
//     sprintf(buff, "PC: $%04x | IR:  %02X   | TCU: %i    |", cpu->pc, cpu->ir, cpu->tcu);
//     mvwaddstr(win_registers, PC_LINE_INDEX, 2, buff);
//     mvwaddch(win_registers, PC_LINE_INDEX, COL_REG_WIDTH_1_3, ACS_VLINE);
//     mvwaddch(win_registers, PC_LINE_INDEX, COL_REG_WIDTH_2_3, ACS_VLINE);
//     mvwaddch(win_registers, PC_LINE_INDEX, COL_REG_WIDTH, ACS_VLINE);

//     mvwaddch(win_registers, PC_LINE_INDEX + 1, 0, ACS_LTEE);
//     mvwhline(win_registers, PC_LINE_INDEX + 1, 1, ACS_HLINE, COL_REG_WIDTH - 1);
//     mvwaddch(win_registers, PC_LINE_INDEX + 1, COL_REG_WIDTH_1_3, ACS_PLUS);
//     mvwaddch(win_registers, PC_LINE_INDEX + 1, COL_REG_WIDTH_2_3, ACS_PLUS);
//     mvwaddch(win_registers, PC_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);

//     const int X_LINE_INDEX = PC_LINE_INDEX + 2;
//     sprintf(buff, " X: $%02x   |  Y: $%02x   |  A: $%02x   |", cpu->x, cpu->y, cpu->a);
//     mvwaddstr(win_registers, X_LINE_INDEX, 2, buff);
//     mvwaddch(win_registers, X_LINE_INDEX, COL_REG_WIDTH_1_3, ACS_VLINE);
//     mvwaddch(win_registers, X_LINE_INDEX, COL_REG_WIDTH_2_3, ACS_VLINE);
//     mvwaddch(win_registers, X_LINE_INDEX, COL_REG_WIDTH, ACS_VLINE);

//     mvwaddch(win_registers, X_LINE_INDEX + 1, 0, ACS_LTEE);
//     mvwhline(win_registers, X_LINE_INDEX + 1, 1, ACS_HLINE, COL_REG_WIDTH - 1);
//     mvwaddch(win_registers, X_LINE_INDEX + 1, COL_REG_WIDTH_1_3, ACS_PLUS);
//     mvwaddch(win_registers, X_LINE_INDEX + 1, COL_REG_WIDTH_2_3, ACS_PLUS);
//     mvwaddch(win_registers, X_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);

//     const int SP_LINE_INDEX = X_LINE_INDEX + 2;
//     sprintf(buff, "SP: $%02x   | PD: $%02x   | CYC: %04li |", cpu->sp, cpu->pd, cpu->cyc);
//     mvwaddstr(win_registers, SP_LINE_INDEX, 2, buff);
//     mvwaddch(win_registers, SP_LINE_INDEX, COL_REG_WIDTH_1_3, ACS_VLINE);
//     mvwaddch(win_registers, SP_LINE_INDEX, COL_REG_WIDTH_2_3, ACS_VLINE);
//     mvwaddch(win_registers, SP_LINE_INDEX, COL_REG_WIDTH, ACS_VLINE);

//     mvwaddch(win_registers, SP_LINE_INDEX + 1, 0, ACS_LTEE);
//     mvwhline(win_registers, SP_LINE_INDEX + 1, 1, ACS_HLINE, COL_REG_WIDTH - 1);
//     mvwaddch(win_registers, SP_LINE_INDEX + 1, COL_REG_WIDTH_1_3, ACS_BTEE);
//     mvwaddch(win_registers, SP_LINE_INDEX + 1, COL_REG_WIDTH_2_3, ACS_BTEE);
//     mvwaddch(win_registers, SP_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);
//     mvwaddch(win_registers, SP_LINE_INDEX + 1, COL_REG_WIDTH_1_4, ACS_TTEE);
//     mvwaddch(win_registers, SP_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_TTEE);
//     mvwaddch(win_registers, SP_LINE_INDEX + 1, COL_REG_WIDTH_3_4, ACS_TTEE);

//     const int P_LINE_INDEX = SP_LINE_INDEX + 2;
//     sprintf(buff, " N: %i  |  V: %i  |  _: %i  |  B: %i",
//         (cpu->p >> 7) & 0b1,
//         (cpu->p >> 6) & 0b1,
//         (cpu->p >> 5) & 0b1,
//         (cpu->p >> 4) & 0b1);
//     mvwaddstr(win_registers, P_LINE_INDEX, 2, buff);
//     mvwaddch( win_registers, P_LINE_INDEX, COL_REG_WIDTH_1_4, ACS_VLINE);
//     mvwaddch( win_registers, P_LINE_INDEX, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch( win_registers, P_LINE_INDEX, COL_REG_WIDTH_3_4, ACS_VLINE);
//     mvwaddch( win_registers, P_LINE_INDEX, COL_REG_WIDTH, ACS_VLINE);

//     mvwaddch(win_registers, P_LINE_INDEX + 1, 0, ACS_LTEE);
//     mvwhline(win_registers, P_LINE_INDEX + 1, 1, ACS_HLINE, COL_REG_WIDTH - 1);
//     mvwaddch(win_registers, P_LINE_INDEX + 1, COL_REG_WIDTH_1_4, ACS_PLUS);
//     mvwaddch(win_registers, P_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_PLUS);
//     mvwaddch(win_registers, P_LINE_INDEX + 1, COL_REG_WIDTH_3_4, ACS_PLUS);
//     mvwaddch(win_registers, P_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);


//     sprintf(buff, " D: %i  |  I: %i  |  Z: %i  |  C: %i",
//         (cpu->p >> 3) & 0b1,
//         (cpu->p >> 2) & 0b1,
//         (cpu->p >> 1) & 0b1,
//         (cpu->p >> 0) & 0b1);
//     mvwaddstr(win_registers, P_LINE_INDEX + 2, 2, buff);
//     mvwaddch( win_registers, P_LINE_INDEX + 2, COL_REG_WIDTH_1_4, ACS_VLINE);
//     mvwaddch( win_registers, P_LINE_INDEX + 2, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch( win_registers, P_LINE_INDEX + 2, COL_REG_WIDTH_3_4, ACS_VLINE);
//     mvwaddch( win_registers, P_LINE_INDEX + 2, COL_REG_WIDTH, ACS_VLINE);

//     mvwaddch(win_registers, P_LINE_INDEX + 3, 0, ACS_LTEE);
//     mvwhline(win_registers, P_LINE_INDEX + 3, 1, ACS_HLINE, COL_REG_WIDTH - 1);
//     mvwaddch(win_registers, P_LINE_INDEX + 3, COL_REG_WIDTH_1_4, ACS_BTEE);
//     mvwaddch(win_registers, P_LINE_INDEX + 3, COL_REG_WIDTH_2_4, ACS_BTEE);
//     mvwaddch(win_registers, P_LINE_INDEX + 3, COL_REG_WIDTH_3_4, ACS_BTEE);
//     mvwaddch(win_registers, P_LINE_INDEX + 3, COL_REG_WIDTH, ACS_RTEE);

//     wrefresh(win_registers);
// }

// void draw_ppu_registers(PPURegisters *ppu) {
//     wclear(win_ppu_registers);
//     box_draw(win_ppu_registers, LEFT, 0, 0, 0, 0);

//     char buff[64];

//     mvwaddch(win_ppu_registers, 0, 0                , ACS_LTEE);
//     mvwaddch(win_ppu_registers, 0, COL_REG_WIDTH_1_4, ACS_BTEE);
//     mvwaddch(win_ppu_registers, 0, COL_REG_WIDTH_2_4, ACS_PLUS);
//     mvwaddch(win_ppu_registers, 0, COL_REG_WIDTH_3_4, ACS_BTEE);
//     mvwaddch(win_ppu_registers, 0, COL_REG_WIDTH    , ACS_RTEE);

//     mvwhline(win_ppu_registers, 1, 1, ACS_HLINE, COL_REG_WIDTH / 2);
//     wattron(win_ppu_registers, COLOR_PAIR(COLOR_NAME));
//     mvwaddstr(win_ppu_registers, 1, 2, "PPU");
//     wattroff(win_ppu_registers, COLOR_PAIR(COLOR_NAME));
//     mvwaddch(win_ppu_registers, 1, 0                , ACS_LTEE);
//     mvwaddch(win_ppu_registers, 1, COL_REG_WIDTH_2_4, ACS_RTEE);

//     const int CTRL_LINE_INDEX = 2;
//     mvwaddstr(win_ppu_registers, CTRL_LINE_INDEX, 2, "$2000 VPHB SINN");
//     sprintf(buff, "$%02x   %i%i%i%i %i%i%i%i",
//         ppu->controller,
//         (ppu->controller >> 7) & 0b1,
//         (ppu->controller >> 6) & 0b1,
//         (ppu->controller >> 5) & 0b1,
//         (ppu->controller >> 4) & 0b1,
//         (ppu->controller >> 3) & 0b1,
//         (ppu->controller >> 2) & 0b1,
//         (ppu->controller >> 1) & 0b1,
//         (ppu->controller >> 0) & 0b1);
//     mvwaddstr(win_ppu_registers, CTRL_LINE_INDEX + 1, 2, buff);

//     mvwaddch(win_ppu_registers, CTRL_LINE_INDEX, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch(win_ppu_registers, CTRL_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch(win_ppu_registers, CTRL_LINE_INDEX + 2, 0, ACS_LTEE);
//     mvwhline(win_ppu_registers, CTRL_LINE_INDEX + 2, 1, ACS_HLINE, COL_REG_WIDTH / 2);
//     mvwaddch(win_ppu_registers, CTRL_LINE_INDEX + 2, COL_REG_WIDTH_2_4, ACS_RTEE);

//     const int MASK_LINE_INDEX = CTRL_LINE_INDEX + 3;
//     mvwaddstr(win_ppu_registers, MASK_LINE_INDEX, 2, "$2001 BGRs bMmG");
//     sprintf(buff, "$%02x   %i%i%i%i %i%i%i%i",
//          ppu->mask,
//         (ppu->mask >> 7) & 0b1,
//         (ppu->mask >> 6) & 0b1,
//         (ppu->mask >> 5) & 0b1,
//         (ppu->mask >> 4) & 0b1,
//         (ppu->mask >> 3) & 0b1,
//         (ppu->mask >> 2) & 0b1,
//         (ppu->mask >> 1) & 0b1,
//         (ppu->mask >> 0) & 0b1);
//     mvwaddstr(win_ppu_registers, MASK_LINE_INDEX + 1, 2, buff);

//     mvwaddch(win_ppu_registers, MASK_LINE_INDEX, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch(win_ppu_registers, MASK_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch(win_ppu_registers, MASK_LINE_INDEX + 2, 0, ACS_LTEE);
//     mvwhline(win_ppu_registers, MASK_LINE_INDEX + 2, 1, ACS_HLINE, COL_REG_WIDTH / 2);
//     mvwaddch(win_ppu_registers, MASK_LINE_INDEX + 2, COL_REG_WIDTH_2_4, ACS_RTEE);

//     const int STATUS_LINE_INDEX = MASK_LINE_INDEX + 3;
//     mvwaddstr(win_ppu_registers, STATUS_LINE_INDEX, 2, "$2002 VSO. ....");
//     sprintf(buff, "$%02x   %i%i%i%i %i%i%i%i",
//          ppu->status,
//         (ppu->status >> 7) & 0b1,
//         (ppu->status >> 6) & 0b1,
//         (ppu->status >> 5) & 0b1,
//         (ppu->status >> 4) & 0b1,
//         (ppu->status >> 3) & 0b1,
//         (ppu->status >> 2) & 0b1,
//         (ppu->status >> 1) & 0b1,
//         (ppu->status >> 0) & 0b1);
//     mvwaddstr(win_ppu_registers, STATUS_LINE_INDEX + 1, 2, buff);

//     mvwaddch(win_ppu_registers, STATUS_LINE_INDEX, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch(win_ppu_registers, STATUS_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_VLINE);
//     mvwaddch(win_ppu_registers, STATUS_LINE_INDEX + 2, 0, ACS_LTEE);
//     mvwhline(win_ppu_registers, STATUS_LINE_INDEX + 2, 1, ACS_HLINE, COL_REG_WIDTH / 2);
//     mvwaddch(win_ppu_registers, STATUS_LINE_INDEX + 2, COL_REG_WIDTH_2_4, ACS_RTEE);

//     const int OAM_ADDR_LINE_INDEX = 1;
//     sprintf(buff, "  OAM ADDR: $%02x", ppu->oam_address);
//     mvwaddstr(win_ppu_registers, OAM_ADDR_LINE_INDEX, COL_REG_WIDTH_2_4 + 2, buff);

//     mvwhline(win_ppu_registers, OAM_ADDR_LINE_INDEX + 1, COL_REG_WIDTH / 2, ACS_HLINE, COL_REG_WIDTH / 2);
//     mvwaddch(win_ppu_registers, OAM_ADDR_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_LTEE);
//     mvwaddch(win_ppu_registers, OAM_ADDR_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);

//     const int OAM_DATA_LINE_INDEX = OAM_ADDR_LINE_INDEX + 2;
//     sprintf(buff, "  OAM DATA: $%02x", ppu->oam_data);
//     mvwaddstr(win_ppu_registers, OAM_DATA_LINE_INDEX, COL_REG_WIDTH_2_4 + 2, buff);

//     mvwhline(win_ppu_registers, OAM_DATA_LINE_INDEX + 1, COL_REG_WIDTH / 2, ACS_HLINE, COL_REG_WIDTH / 2);
//     mvwaddch(win_ppu_registers, OAM_DATA_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_LTEE);
//     mvwaddch(win_ppu_registers, OAM_DATA_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);

//     const int SCROLL_LINE_INDEX = OAM_DATA_LINE_INDEX + 2;
//     sprintf(buff, "$2005 SCRL: $%02x", ppu->scroll);
//     mvwaddstr(win_ppu_registers, SCROLL_LINE_INDEX, COL_REG_WIDTH_2_4 + 2, buff);

//     mvwhline(win_ppu_registers, SCROLL_LINE_INDEX + 1, COL_REG_WIDTH / 2, ACS_HLINE, COL_REG_WIDTH / 2);
//     mvwaddch(win_ppu_registers, SCROLL_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);
//     mvwaddch(win_ppu_registers, SCROLL_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_LTEE);

//     const int ADDR_LINE_INDEX = SCROLL_LINE_INDEX + 2;
//     sprintf(buff, "$2006 ADDR: $%02x", ppu->address);
//     mvwaddstr(win_ppu_registers, ADDR_LINE_INDEX, COL_REG_WIDTH_2_4 + 2, buff);

//     mvwhline(win_ppu_registers, ADDR_LINE_INDEX + 1, COL_REG_WIDTH / 2, ACS_HLINE, COL_REG_WIDTH / 2);
//     mvwaddch(win_ppu_registers, ADDR_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);
//     mvwaddch(win_ppu_registers, ADDR_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_LTEE);

//     const int DATA_LINE_INDEX = ADDR_LINE_INDEX + 2;
//     sprintf(buff, "$2007 DATA: $%02x", ppu->data);
//     mvwaddstr(win_ppu_registers, DATA_LINE_INDEX, COL_REG_WIDTH_2_4 + 2, buff);

//     mvwhline(win_ppu_registers, DATA_LINE_INDEX + 1, 1, ACS_HLINE, COL_REG_WIDTH);
//     mvwaddch(win_ppu_registers, DATA_LINE_INDEX + 1, COL_REG_WIDTH, ACS_RTEE);
//     mvwaddch(win_ppu_registers, DATA_LINE_INDEX + 1, COL_REG_WIDTH_2_4, ACS_BTEE);
//     mvwaddch(win_ppu_registers, DATA_LINE_INDEX + 1, 0, ACS_LTEE);

//     wrefresh(win_ppu_registers);
// }

// void box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr) {
//     if (ul == 0)
//         ul = ((intersect & UP) == UP) && ((intersect & LEFT) == LEFT) ? ACS_PLUS : ((intersect & UP) == UP) ? ACS_LTEE : ((intersect & LEFT) == LEFT) ? ACS_TTEE : ACS_ULCORNER;

//     if (ur == 0)
//         ur = ((intersect & UP) == UP) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS : ((intersect & UP) == UP) ? ACS_RTEE : ((intersect & RIGHT) == RIGHT) ? ACS_TTEE : ACS_URCORNER;

//     if (ll == 0)
//         ll = ((intersect & DOWN) == DOWN) && ((intersect & LEFT) == LEFT) ? ACS_PLUS : ((intersect & DOWN) == DOWN) ? ACS_LTEE : ((intersect & LEFT) == LEFT) ? ACS_BTEE : ACS_LLCORNER;

//     if (lr == 0)
//         lr = ((intersect & DOWN) == DOWN) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS : ((intersect & DOWN) == DOWN) ? ACS_RTEE : ((intersect & RIGHT) == RIGHT) ? ACS_BTEE : ACS_LRCORNER;

//     wborder(win,
//             ACS_VLINE,
//             ACS_VLINE,
//             ACS_HLINE,
//             ACS_HLINE,
//             ul,
//             ur,
//             ll,
//             lr);
// }
