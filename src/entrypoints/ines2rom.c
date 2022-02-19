#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"

typedef enum {
    M_HORIZONTAL  = 0,
    M_VERTICAL    = 1,
    M_FOUR_SCREEN = 2,
} PPUMirroring;

// typedef enum {
//     TV_NTSC = 0,
//     TV_PAL = 2,

//     TV_DUAL = 1,// 3
// } TVSystem;

// TODO: Create direct load as well instead of bin -> ascii -> bin
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: ines2rom <src> <prg-dest>\n");
        return 1;
    }
    FILE *ines;
    FILE *rom_prg;
    // FILE *rom_chr;
    if (!(ines = fopen(argv[1], "r"))) {
        fprintf(stderr, "Failed to open iNES '%s'\n", argv[1]);
        return 1;
    }
    if (!(rom_prg = fopen(argv[2], "w+"))) {
        fprintf(stderr, "Failed to open PRG ROM '%s'\n", argv[2]);
        return 1;
    }
    // if (!(rom_chr = fopen(argv[3], "w+"))) {
    //     fprintf(stderr, "Failed to open CHR ROM '%s'\n", argv[2]);
    //     return 1;
    // }

    u8 header[16];
    if (fread(header, 1, 16, ines) != 16) {
        fprintf(stderr, "Could not read 16 byte header\n");
        return 1;
    }

    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1A) {
        fprintf(stderr, "Did not match iNES header\n");
        return 1;
    }

    u8 prg_size_val = header[4];
    u8 chr_size_val = header[5];

    u8           flags6          = header[6];
    PPUMirroring mirroring       = flags6 & 0x01;
    bool         hasCartridgeRAM = (flags6 & 0x02) == 0x02;
    bool         hasTrainer      = (flags6 & 0x04) == 0x04;
    if ((flags6 & 0x08) == 0x08)
        mirroring = M_FOUR_SCREEN;
    u8 mapper = (flags6 >> 4);

    u8   flags7          = header[7];
    bool hasVSUnisystem  = (flags7 & 0x01) == 0x01;
    bool hasPlayChoice10 = (flags7 & 0x02) == 0x02;
    bool isNES2          = ((flags7 & 0x0F) >> 2) == 2;
    mapper |= flags7 & 0xF0;

    u8 prg_ram_size = header[8];

    // u8 flags9 = header[9];
    // u8 flags10 = header[10];

    printf("Read iNES header.\n");
    printf("    PRG ROM SIZE:  %i (%i)\n", prg_size_val, prg_size_val * 0x4000);
    printf("    CHR ROM SIZE:  %i (%i)\n", chr_size_val, chr_size_val * 0x2000);
    printf("    Mirroring:     %c\n", mirroring == M_HORIZONTAL ? 'H' : mirroring == M_VERTICAL ? 'V' : '4');
    printf("    Cartridge RAM: %i\n", hasCartridgeRAM);
    printf("    Trainer:       %i\n", hasTrainer);
    printf("    VS Unisystem:  %i\n", hasVSUnisystem);
    printf("    PlayChoice-10: %i\n", hasPlayChoice10);
    printf("    NES 2.0:       %i\n", isNES2);
    printf("    Mapper:        %i\n", mapper);
    printf("    PRG RAM SIZE:  %i (%i)\n", prg_ram_size, prg_ram_size * 0x2000);

    if (isNES2) {
        fprintf(stderr, "NES 2.0 not supported\n");
        return 1;
    }
    if (hasCartridgeRAM || hasTrainer || hasVSUnisystem || hasPlayChoice10) {
        fprintf(stderr, "Contains unsupported options.\n");
        return 1;
    }
    if (mapper != 0) {
        fprintf(stderr, "Unsupported Mapper.\n");
        return 1;
    }

    // printf("    FLAGS9 %02X\n", flags9);
    // printf("    FLAGS10 %02X\n", flags10);

    // I don't look at or support flags because the 1 file I need doesn't use them.

    u8 buff[16];

    fprintf(rom_prg, "8000:\n");
    fprintf(rom_prg, "# Converted from %s (PRG-ROM)\n", argv[1]);
    for (int i = 0; i < 1024 * prg_size_val; i++) {
        fread(buff, 1, 16, ines);
        for (int i = 0; i < 16; i++) {
            fprintf(rom_prg, "%02X ", buff[i]);
        }
        fprintf(rom_prg, "# $%04x\n", 0x8000 + i * 16);
    }

    if (prg_size_val == 1) {
        fprintf(rom_prg, "\n# Begin Mirroring\n\n");
        fseek(ines, 16, SEEK_SET); // move to beginning of PRG ROM
        for (int i = 0; i < 1024 * prg_size_val; i++) {
            fread(buff, 1, 16, ines);
            for (int i = 0; i < 16; i++) {
                fprintf(rom_prg, "%02X ", buff[i]);
            }
            fprintf(rom_prg, "# $%04x\n", 0xC000 + i * 16);
        }
    }

    // no CHR ROM
    // u16 chr_start = prg_size_val * 0x4000 + 0x4020;
    // fprintf(rom_chr, "%04X:\n", chr_start);
    // fprintf(rom_chr, "# Converted from %s (CHR-ROM)\n", argv[1]);
    // for (int i = 0; i < 1024 * prg_size_val; i++){
    //     fread(buff, 1, 16, ines);
    //     for (int i = 0; i < 16; i++) {
    //         fprintf(rom_chr, "%02X ", buff[i]);
    //     }
    //     fprintf(rom_chr, "# $%04x$%04x\n", chr_start + i*16);
    // }

    fclose(ines);
    fclose(rom_prg);
    // fclose(rom_chr);
}