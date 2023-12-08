#pragma once

#include <common.h>

inline void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++)
        pdest[i] = psrc[i];
    
    return dest;
}

inline void *memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++)
        p[i] = (uint8_t)c;
    
    return s;
}

inline void *memmove(void *dest, const void *src, size_t n)
{
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest)
    {
        for (size_t i = 0; i < n; i++)
            pdest[i] = psrc[i];
    }
    else if (src < dest)
    {
        for (size_t i = n; i > 0; i--)
            pdest[i - 1] = psrc[i - 1];
    }

    return dest;
}

inline int memcmp(const void *s1, const void *s2, size_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
            return p1[i] < p2[i] ? -1 : 1;
    }
    
    return 0;
}

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

inline char *strncpy(char *dst, const char *src, size_t n)
{
    if (!dst)
        return NULL;
    
    char *ptr = dst;
    while (*src && n--)
        *dst++ = *src++;
    
    *dst = '\0';
    return ptr;
}

char *strdup(const char *s);

inline int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n);

inline char *strcat(char *s1, const char *s2)
{
    char *ret = s1;
    strcpy(s1 + strlen(s1), s2);
    
    return ret;
}

inline char *strncat(char *s1, const char *s2, size_t n)
{
    char *ret = s1;
    strncpy(s1 + strlen(s1), s2, n);
    
    return ret;
}