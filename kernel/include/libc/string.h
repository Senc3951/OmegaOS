#pragma once

#include <common.h>

inline void *memset(void *dst, const int val, size_t len)
{
    uint8_t *ptr = (uint8_t *)dst;
    while (len-- > 0)
        *ptr++ = val;
    
    return dst;
}