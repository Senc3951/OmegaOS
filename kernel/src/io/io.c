#include <io/io.h>

void outb(const uint16_t port, const uint8_t value)
{
    asm volatile("outb %1, %0" : : "dN"(port), "a"(value)); \
}

void outw(const uint16_t port, const uint16_t value)
{
    asm volatile("outw %%ax, %%dx" : : "d"(port), "a"(value));
}

void outl(const uint16_t port, const uint32_t value)
{
    asm volatile("outl %0, %1" : : "a"(value) , "Nd"(port));
}

void outsm(const uint16_t port, uint8_t *data, uint32_t size)
{
    asm volatile("rep outsw" : "+S"(data), "+c"(size) : "d"(port));
}

uint8_t inb(const uint16_t port)
{
    uint8_t val;
    asm volatile("inb %1, %0"
                    : "=a"(val)
                    : "Nd"(port));
    return val;
}

uint16_t inw(const uint16_t port)
{
    uint16_t val;
    asm volatile("inw %1, %0"
                    : "=a"(val)
                    : "Nd"(port));
    return val;
}

uint32_t inl(const uint16_t port)
{
    uint32_t val;
    asm volatile("inl %1, %0"
                    : "=a"(val)
                    : "Nd"(port));
    return val;
}

void insm(const uint16_t port, uint8_t *data, uint32_t size)
{
    asm volatile("rep insw" : "+D"(data), "+c"(size) : "d"(port) : "memory");
}