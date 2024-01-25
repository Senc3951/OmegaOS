#include <dev/ps2/translator.h>

#define NUM_START       0x2
#define NUM_END         0xB
#define LETTER_START    0xC
#define LETTER_END      0x39

static const char g_LETTERS[] = {
    '-', '=',
    NO_KEY,
    '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n',
    NO_KEY,
    'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', NO_KEY, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm',
    ',', '.', '/', NO_KEY, NO_KEY, NO_KEY, ' '
};
static const char g_CAPS_LETTERS[] = {
    '-', '=',
    NO_KEY,
    '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
    '\n',
    NO_KEY,
    'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ';', '\'', '`', NO_KEY, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',
    ',', '.', '/', NO_KEY, NO_KEY, NO_KEY, ' '
};
static const char g_SHIFT_LETTERS[] = {
    '_', '+',
    NO_KEY,
    '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n',
    NO_KEY,
    'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ':', '"', '~', NO_KEY, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',
    '<', '>', '?', NO_KEY, NO_KEY, NO_KEY, ' '
};
static const char g_NUMBERS[] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'
};
static const char g_SYMBOLS[] = {
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')'
};

char translate(const uint8_t code, const bool shift, const bool caps)
{
    if (code >= NUM_START && code <= NUM_END)
    {
        if (shift)
            return g_SYMBOLS[code - NUM_START];
        
        return g_NUMBERS[code - NUM_START];
    }
    if (code >= LETTER_START && code <= LETTER_END)
    {
        if (caps)
            return g_CAPS_LETTERS[code - LETTER_START];
        else if (shift)
            return g_SHIFT_LETTERS[code - LETTER_START];

        return g_LETTERS[code - LETTER_START];
    }
    
    return NO_KEY;
}