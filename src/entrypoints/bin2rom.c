#include "stdio.h"
#include "stdint.h"

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
    // fwrite("0000:\n", 1, 6, rom);
    uint16_t label = 0;
    uint8_t buff[16];
    // char strbuff[16];
    int n = fread(buff, 1, 16, bin);
    while (n > 0) {
        for (int i = 0; i < n; i++) {
            fprintf(rom, "%02X ", buff[i]);
            // sprintf(strbuff, "%02X ", buff[i]);
            // fwrite(strbuff, 1, 3, rom);
        }
        fprintf(rom, "# $%04x\n", label);
        // sprintf(strbuff, "# $%04x\n", label);
        // fwrite(strbuff, 1, 7, rom);

        label += n;
        n = fread(buff, 1, 16, bin);
    }

    fclose(bin);
    fclose(rom);
}