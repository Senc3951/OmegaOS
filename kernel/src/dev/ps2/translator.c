#include <dev/ps2/translator.h>

#define BASIC_KEY_START 0x0D
#define BASIC_KEY_END   0x5E

const uint8_t g_LETTERS_NUMBERS[] = {
    '`',
    NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY, NO_KEY,
    'q',
    '1',
    NO_KEY, NO_KEY, NO_KEY,
    'z',
    's',
    'a',
    'w',
    '2',
    NO_KEY, NO_KEY,
    'c',
    'x',
    'd',
    'e',
    '4',
    '3',
    NO_KEY, NO_KEY,
    ' ',
    'v',
    'f',
    't',
    'r',
    '5',
    NO_KEY, NO_KEY,
    'n',
    'b',
    'h',
    'g',
    'y',
    '6',
    NO_KEY, NO_KEY, NO_KEY,
    'm',
    'j',
    'u',
    '7',
    '8',
    NO_KEY, NO_KEY,
    ',',
    'k',
    'i',
    'o',
    '0',
    '9',
    NO_KEY, NO_KEY,
    '.',
    '/',
    'l',
    ';',
    'p',
    '-',
    NO_KEY, NO_KEY, NO_KEY,
    '\'',
    NO_KEY,
    '[',
    '=',
    NO_KEY, NO_KEY, NO_KEY, NO_KEY,
    '\n',
    ']',
    NO_KEY,
    '\\'
};

const uint8_t g_SYMBOLS[] = {
    ')',
    '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '('
};

uint8_t translate(const uint8_t code, const bool caps, const bool shift)
{
    if (code > BASIC_KEY_START && code < BASIC_KEY_END)
    {
        uint8_t c = g_LETTERS_NUMBERS[code - BASIC_KEY_START - 1];
        if (c != NO_KEY)
        {
            if (shift)
            {
                switch (c)
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        return g_SYMBOLS[c - '0'];
                    case '-':
                        return '_';
                    case '=':
                        return '+';
                    case '\n':
                        return '\n';
                    case ' ':
                        return ' ';
                    case '\\':
                        return '|';
                    case '`':
                        return '~';
                    case ';':
                        return ':';
                    case '\'':
                        return '"';
                    case ',':
                        return '<';
                    case '.':
                        return '>';
                    case '/':
                        return '?';
                    case '[':
                        return '{';
                    case ']':
                        return '}';
                    default:
                        return c - 32;
                }
            }
            if (caps)
            {
                if (c >= 'a' && c <= 'z')
                    return c - 32;
            }
            
            return c;
        }
    }
    else
    {
        if (code == TAB && !shift)
            return '\t';
    }
    
    return NO_KEY;
}