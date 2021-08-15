#include "stdio.h"
#include "stdint.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: ines2rom <src> <prg-dest> <chr-dest>\n");
        return 1;
    }
    FILE *ines;
    FILE *rom_prg;
    FILE *rom_chr;
    if (!(ines = fopen(argv[1], "r"))) {
        fprintf(stderr, "Failed to open iNES '%s'\n", argv[1]);
        return 1;
    }
    if (!(rom_prg = fopen(argv[2], "w+"))) {
        fprintf(stderr, "Failed to open PRG ROM '%s'\n", argv[2]);
        return 1;
    }
    if (!(rom_chr = fopen(argv[3], "w+"))) {
        fprintf(stderr, "Failed to open CHR ROM '%s'\n", argv[2]);
        return 1;
    }

    uint8_t header[16];
    if (fread(header, 1, 16, ines) != 16) {
        fprintf(stderr, "Could not read 16 byte header\n");
    }

    if (header[0] != 'N' ||
        header[1] != 'E' ||
        header[2] != 'S' ||
        header[3] != 0x1A)
    {
        fprintf(stderr, "Did not match iNES header\n");
    }

    uint8_t prg_size_val = header[4];
    uint8_t chr_size_val = header[5];
    uint8_t flags6 = header[6];
    uint8_t flags7 = header[7];
    uint8_t flags8 = header[8];
    uint8_t flags9 = header[9];
    uint8_t flags10 = header[10];

    printf("Read iNES header.\n");
    printf("    PRG SIZE: %i (%i)\n", prg_size_val, prg_size_val * 0x4000);
    printf("    CHR SIZE: %i (%i)\n", chr_size_val, chr_size_val * 0x2000);
    printf("    FLAGS6 %02X\n", flags6);
    printf("    FLAGS7 %02X\n", flags7);
    printf("    FLAGS8 %02X\n", flags8);
    printf("    FLAGS9 %02X\n", flags9);
    printf("    FLAGS10 %02X\n", flags10);

    // I don't look at or support flags because the 1 file I need doesn't use them.

    uint8_t buff[16];

    fprintf(rom_prg, "4020:\n");
    fprintf(rom_prg, "# Converted from %s (PRG-ROM)\n", argv[1]);
    for (int i = 0; i < 1000 * prg_size_val; i++){
        fread(buff, 1, 16, ines);
        for (int i = 0; i < 16; i++) {
            fprintf(rom_prg, "%02X ", buff[i]);
        }
        fprintf(rom_prg, "# $%04x\n", i*16);
    }

    fprintf(rom_chr, "%04X:\n", prg_size_val*0x4000+0x4020);
    fprintf(rom_chr, "# Converted from %s (CHR-ROM)\n", argv[1]);
    for (int i = 0; i < 1000 * prg_size_val; i++){
        fread(buff, 1, 16, ines);
        for (int i = 0; i < 16; i++) {
            fprintf(rom_chr, "%02X ", buff[i]);
        }
        fprintf(rom_chr, "# $%04x\n", i*16);
    }

    fclose(ines);
    fclose(rom_prg);
    fclose(rom_chr);
}