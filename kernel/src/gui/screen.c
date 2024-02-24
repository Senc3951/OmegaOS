#include <gui/screen.h>
#include <gui/printf.h>
#include <libc/string.h>
#include <logger.h>

static Framebuffer_t* g_fb;
static PSF1Font_t* g_font;
static uint32_t g_fbPitch;
static uint32_t g_x, g_y, g_fg, g_bg;

#define BYTE_SIZE       8
#define BYTES_PER_CHAR  16

#define ROW_SIZE    (g_fb->width * g_fb->bytesPerPixel)
#define COL_SIZE    (g_fb->height * g_fb->bytesPerPixel * 8 / BYTES_PER_CHAR)
#define ROW_COUNT   (g_fb->height / BYTE_SIZE)
#define COL_COUNT   (g_fb->width / BYTES_PER_CHAR)

static inline void drawPixel(const uint32_t x, const uint32_t y, const uint32_t color)
{
    *(uint32_t *)((uint64_t)g_fb->baseAddress + y * g_fbPitch + x * g_fb->bytesPerPixel) = color;
}

static void drawc(const char c)
{
    char *fontPtr = (char *)g_font->glyphBuffer + (c * g_font->header->charsize);
    for (uint32_t y = g_y; y < g_y + BYTES_PER_CHAR; y++)
    {
        for (uint32_t x = g_x; x < g_x + BYTE_SIZE; x++)
        {
            if ((*fontPtr & (0b10000000 >> (x - g_x))) > 0)
                drawPixel(x, y, g_fg);
            else
                drawPixel(x, y, g_bg);
        }
                
        fontPtr++;
    }
    
    g_x += BYTE_SIZE;
    if (g_x + BYTE_SIZE > g_fb->width)
    {
        g_x = 0;
        g_y += BYTES_PER_CHAR;
    }
}

void screen_init(Framebuffer_t *fb, PSF1Font_t *font)
{
    g_fb = fb;
    g_font = font;
    g_fbPitch = g_fb->bytesPerPixel * g_fb->pixelsPerScanLine;
    g_fg = White;
    g_bg = Black;
    
    screen_clear(Black);
    LOG("Framebuffer at %p (%ux%ux%u)\n", fb->baseAddress, fb->height, fb->width, fb->bytesPerPixel * 8);
}

static void scroll()
{
    screen_clear(Black);
}

void screen_putc(const char c)
{
    if (g_y + BYTES_PER_CHAR > g_fb->height)
        scroll();
    
    switch (c)
    {
        case '\t':
            g_x += BYTE_SIZE * CHARS_PER_TAB;
            if (g_x + BYTE_SIZE <= g_fb->width)
                break;
        case '\n':
            g_x = 0;
            g_y += BYTES_PER_CHAR;
            
            break;
        case '\b':
            if (g_y == 0 && g_x == 0)
                break;
            if (g_x == 0)
            {
                g_x = g_fb->width;
                g_y--;
            }
            else
                g_x -= BYTE_SIZE;
            
            break;
        case '\0':
            break;
        default:
            drawc(c);
            break;
    }
}

void screen_puts(const char* s)
{
    while (*s)
        screen_putc(*s++);
}

void kprintf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vprintf_(fmt, va);   
    va_end(va);
}

void kvprintf(va_list va, const char *fmt)
{
    vprintf_(fmt, va);   
}

void screen_clear(const uint32_t color)
{
    g_bg = color;
    if (color == Black)
        memset(g_fb->baseAddress, 0, g_fb->bufferSize);
    else
        screen_fillrect(0, 0, g_fb->width, g_fb->height, color);
    
    g_x = g_y = 0;
}

void screen_fillrect(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color)
{
    for (uint32_t i = x; i < x + w; i++)
    {
        for (uint32_t j = y; j < y + h; j++)
            drawPixel(i, j, color);
    }
}