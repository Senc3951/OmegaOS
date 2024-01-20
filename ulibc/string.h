#pragma once

#include <stdint.h>
#include <stddef.h>

inline size_t strlen(const char *s)
{
    const char *str;
    for (str = s; *str; str++) ;
    
    return str - s;
}

inline char *strcpy(char *dst, const char *src)
{
    if (!dst)
        return NULL;
    
    char *ptr = dst;
    while (*src)
        *dst++ = *src++;
    
    *dst = '\0';
    return ptr;
}