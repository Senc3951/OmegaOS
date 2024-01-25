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
    for ( ; n--; s1++, s2++)
    {
        if (*s1 != *s2)
            return *s1 - *s2;
    }
    
    return 0;
}

char *strtok(char *s, const char *delim)
{
    static char *p = NULL;
    if (!s && ((s = p) == NULL))
        return NULL;
    
    s += strspn(s, delim);
    if (!*s)
        return p = NULL;
    
    p = s + strcspn(s, delim);
    if (*p)
        *p++ = '\0';
    else 
        p = NULL;
    
    return s;
}