#include <libc/string.h>
#include <mem/heap.h>

char *strdup(const char *s)
{
    size_t len = strlen(s);
    char *copy = (char *)kmalloc(len + 1);
    if (!copy)
        return NULL;
    
    strncpy(copy, s, len);
    return copy;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while (n-- && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    
    if (n)
        return *(const unsigned char *)s1 - *(const unsigned char *)s2;
    
    return 0;
}