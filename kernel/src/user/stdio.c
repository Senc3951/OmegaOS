#include <stdio.h>
#include <string.h>
#include <unistd.h>

FILE _stdin  = { .fd = 0 };
FILE _stdout = { .fd = 1 };
FILE _stderr = { .fd = 2 };

int getc(FILE *file)
{
    char c;
    do
    {
        if (read(file->fd, &c, 1) == 1)
            break;
    } while (1);
    
    putc(c, stdout);
    return c;
}

int getchar()
{
    return getc(stdin);
}

char *gets(char *buf)
{
    size_t i = 0;
    char c;
    
    do
    {
        c = getc(stdin);
        if (c == '\n' || c == '\0')
            break;

        buf[i++] = c;
    } while (1);
    
    buf[i] = '\0';
    return buf;
}

int putc(const int c, FILE *file)
{
    char pc = c;
    if (write(file->fd, &pc, 1) == 1)
        return c;

    return 0;
}

int putchar(const int c)
{
    return putc(c, stdout);
}

int puts(const char *s)
{
    return write(_stdout.fd, s, strlen(s));
}