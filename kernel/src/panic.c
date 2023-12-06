#include <panic.h>
#include <gui/screen.h>
#include <io/io.h>

void __NO_RETURN__  __kpanic(const char *file, const char *function, const uint64_t line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    
    screen_clear(Blue);
    screen_puts("Panic: ");
    kvprintf(va, fmt);
    kprintf(", function %s, file %s, line %llu\n", function, file, line);
    va_end(va);
    
    __HCF();
}