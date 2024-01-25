#pragma once

#define SEEK_SET	0	/* Seek from beginning of file. */
#define SEEK_CUR	1	/* Seek from current position. */
#define SEEK_END	2	/* Seek from end of file. */

#define stdin   (&_stdin)
#define stdout  (&_stdout)
#define stderr  (&_stderr)

typedef struct FILE
{
    int fd;
} FILE;

extern FILE _stdin;
extern FILE _stdout;
extern FILE _stderr;

int getc(FILE *file);
int getchar();
char *gets(char *buf);
int putc(const int c, FILE *file);
int putchar(const int c);
int puts(const char *s);