#include "stdio.h"
#include "stdint.h"

// TODO: Create direct load as well instead of bin -> ascii -> bin
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: bin2rom <src> <dest>\n");
        return 1;
    }
    FILE *bin;
    FILE *rom;
    if (!(bin = fopen(argv[1], "r"))) {
        fprintf(stderr, "Failed to open bin '%s'\n", argv[1]);
        return 1;
    }
    if (!(rom = fopen(argv[2], "w+"))) {
        fprintf(stderr, "Failed to open rom '%s'\n", argv[2]);
        return 1;
    }

    fprintf(rom, "0000:\n");
    fprintf(rom, "# Converted from %s\n", argv[1]);
    uint16_t label = 0;
    uint8_t buff[16];
    int n = fread(buff, 1, 16, bin);
    while (n > 0) {
        for (int i = 0; i < n; i++) {
            fprintf(rom, "%02X ", buff[i]);
        }
        fprintf(rom, "# $%04x\n", label);
        label += n;
        n = fread(buff, 1, 16, bin);
    }

    fclose(bin);
    fclose(rom);
}