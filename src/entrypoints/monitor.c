#include "stdio.h"
#include "signal.h"
#include "ncurses.h"
#include "../headers/rom.h"

void intHandle(int sig);

int main() {
    // signal(SIGINT, intHandle);

    // initscr();
    // curs_set(0);
    // noecho();
    // raw();
    // start_color();
    // use_default_colors();
    // wrefresh(stdscr);

    // char ch;
    // while (1)
    // {
    //     ch = getch();
    //     switch (ch)
    //     {
    //     case 0x03:
    //         raise(SIGINT);
    //         return 0;
    //     }
    // }

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
}

void intHandle(int sig) {
    endwin();
}