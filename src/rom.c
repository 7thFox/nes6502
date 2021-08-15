#include "headers/rom.h"

#define uint unsigned int

u8 parse_byte(const char *contents, uint *i);
u8 get_char_value(char c);
void remove_trivia(const char *contents, uint *i, size_t l);

bool rom_load(Rom *rom, const char *filepath) {
    tracef("rom_load \n");
    rom->value = 0;
    FILE *f = fopen(filepath, "r");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    char *contents = malloc(size);
    fseek(f, 0, SEEK_SET);
    fread(contents, 1, size, f);
    fclose(f);

    if (size < 5) return false;

    size_t cap = 255;
    rom->value = malloc(sizeof(u8) * cap);
    rom->rom_size = 0;

    uint i = 0;
    rom->map_offset = parse_byte(contents, &i) << 8;
    rom->map_offset |= parse_byte(contents, &i);
    if (contents[i] != ':') return false;
    i++;// :

    while (i < size) {
        remove_trivia(contents, &i, size);
        if (i + 1u >= size) break;

        if (size >= cap) {
            cap += 255;
            rom->value = realloc(rom->value, sizeof(u8) * cap);
        }

        rom->value[rom->rom_size] = parse_byte(contents, &i);
        rom->rom_size++;
    }

    logf("Successfully loaded ROM '%s' with %li bytes ($%04x-$%04lx)\n", filepath, rom->rom_size, rom->map_offset, rom->map_offset + rom->rom_size - 1);

    return true;
}

u8 parse_byte(const char *contents, uint *i) {
    char c = contents[*i];
    (*i)++;

    u8 val = get_char_value(c) << 4;

    c = contents[*i];
    (*i)++;
    val |= get_char_value(c);

    return val;
}

u8 get_char_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

void remove_trivia(const char *contents, uint *i, size_t l) {
    while (*i < l) {
        switch (contents[*i])
        {
        case '\n':
        case ' ':
        case '\t':
        case '\r':
            (*i)++;
            break;
        case '#':
            (*i)++;
            while (*i < l && contents[*i] != '\n') (*i)++;
            break;
        default:
            return;
        }
    }
}
