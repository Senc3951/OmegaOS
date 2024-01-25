#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

void shell()
{
    puts("Hello from userspace\n");
    
    char line[100];
    while (1)
        gets(line);
    
    exit(0);
}