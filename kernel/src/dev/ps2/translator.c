#include <dev/ps2/translator.h>

#define NUM_START       0x2
#define NUM_END         0xB
#define LETTER_START    0xC
#define LETTER_END      0x39

const char g_LETTERS[] = {
    '-', '=',
    NO_KEY,
    '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', ']', ']',
    '\n',
    NO_KEY,
    'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', NO_KEY, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm',
    ',', '.', '/', NO_KEY, NO_KEY, NO_KEY, ' '
};
const char g_NUMBERS[] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
};

char translate(const uint8_t code)
{
    if (code >= NUM_START && code <= NUM_END)
        return g_NUMBERS[code - NUM_START];
    if (code >= LETTER_START && code <= LETTER_END)
        return g_LETTERS[code - LETTER_START];
    
    return NO_KEY;
}