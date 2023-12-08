#include <gui/screen.h>
#include <gui/printf.h>
#include <libc/string.h>

static Framebuffer_t* g_fb;
static PSF1Font_t* g_font;
static uint32_t g_fbPitch;
static uint32_t g_x, g_y, g_color;

static void drawPixel(const uint32_t x, const uint32_t y, const uint32_t color)
{
    *(uint32_t *)((uint64_t)(g_fb->baseAddress) + y * g_fbPitch + x * g_fb->bytesPerPixel) = color;
}

static void drawc(const char c)
{
    char* fontPtr = (char *)g_font->glyphBuffer + (c * g_font->header->charsize);
    for (uint32_t y = g_y; y < g_y + 16; y++)
    {
        for (uint32_t x = g_x; x < g_x + 8; x++)
        {
            if ((*fontPtr & (0b10000000 >> (x - g_x))) > 0)
                drawPixel(x, y, g_color);
        }
                
        fontPtr++;
    }
    
    g_x += 8;
    if (g_x + 8 > g_fb->width)
    {
        g_x = 0;
        g_y += 16;
    }
}

void screen_init(Framebuffer_t *fb, PSF1Font_t *font)
{
    g_fb = fb;
    g_font = font;
    g_fbPitch = g_fb->bytesPerPixel * g_fb->pixelsPerScanLine;
    g_color = White;
    
    screen_clear(Black);
}

void screen_putc(const char c)
{
    switch (c)
    {
        case '\t':
            g_x += 8 * CHARS_PER_TAB;
            if (g_x + 8 <= g_fb->width)
                break;
        case '\n':
            g_x = 0;
            g_y += 16;
            
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