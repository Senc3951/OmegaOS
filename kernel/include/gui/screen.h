#pragma once

#include <bootinfo.h>

/* Amount of characters to skip on a tab. */
#define CHARS_PER_TAB 4

enum
{
    Black   = 0,
    Blue    = 0x0000FF,
    Green   = 0x00FF00,
    Cyan    = 0x00FFFF,
    Red     = 0xFF0000,
    Yellow  = 0xFFFF00,
    White   = 0xFFFFFF
};

/// @brief Initialize the screen.
/// @param fb Framebuffer information.
/// @param font Font information.
void screen_init(Framebuffer_t *fb, PSF1Font_t *font);

/// @brief Print a char to the screen, including special ones line NL.
/// @param c Char to print.
void screen_putc(const char c);

/// @brief Print a string to the screen.
/// @param s String to print.
void screen_puts(const char *s);

/// @brief Print a formatted string to the screen.
/// @param fmt Format of the string.
void kprintf(const char *fmt, ...);

/// @brief Print a formatted string to the screen.
/// @param va List of parameters.
/// @param fmt Format of the string.
void kvprintf(va_list va, const char *fmt);

/// @brief Clear the screen with a specific color.
/// @param color Color to fill the screen with.
void screen_clear(const uint32_t color);

/// @brief Draw a rect on the screen.
/// @param x Start x coordinate.
/// @param y Start y coordinate.
/// @param w Width of the rect.
/// @param h Height of the rect.
/// @param color Color of the rect.
void screen_fillrect(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color);