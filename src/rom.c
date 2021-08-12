#include "headers/rom.h"

uint8_t parse_byte(const char *contents, int *i, size_t l);
uint8_t get_char_value(char c);
void remove_trivia(const char *contents, int *i, size_t l);

bool rom_load(Rom *rom, const char *filepath) {
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

    if (size < 3) return false;

    size_t cap = 255;
    rom->value = malloc(sizeof(uint8_t) * cap);
    rom->rom_size = 0;

    int i = 0;
    rom->map_offset = parse_byte(contents, &i, size) << 8;
    rom->map_offset |= parse_byte(contents, &i, size);
    if (contents[i] != ':') return false;
    i++;// :

    while (i < size) {
        remove_trivia(contents, &i, size);
        if (i + 1 >= size) break;

        if (size >= cap) {
            cap += 255;
            rom->value = realloc(rom->value, sizeof(uint8_t) * cap);
        }

        rom->value[rom->rom_size] = parse_byte(contents, &i, size);
        rom->rom_size++;
    }

    return true;
}

uint8_t parse_byte(const char *contents, int *i, size_t l) {
    char c = contents[*i];
    (*i)++;

    uint8_t val = get_char_value(c) << 4;

    c = contents[*i];
    (*i)++;
    val |= get_char_value(c);

    return val;
}

uint8_t get_char_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

void remove_trivia(const char *contents, int *i, size_t l) {
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
