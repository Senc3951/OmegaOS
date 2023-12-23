#pragma once

#include <common.h>

/// @brief Write a byte to an io port.
/// @param port Port to write the value to.
/// @param value Value to write.
void outb(const uint16_t port, const uint8_t value);

/// @brief Write a word to an io port.
/// @param port Port to write the value to.
/// @param value Value to write.
void outw(const uint16_t port, const uint16_t value);

/// @brief Write a dword to an io port.
/// @param port Port to write the value to.
/// @param value Value to write.
void outl(const uint16_t port, const uint32_t value);

/// @brief Write a word x times to an io port.
/// @param port Port to write the value to.
/// @param data Data to write.
/// @param size Amount of words to write.
void outsm(const uint16_t port, uint8_t *data, uint32_t size);

/// @brief Read a byte from an io port.
/// @param port Port to read from.
/// @return Value that was read from the port.
uint8_t inb(const uint16_t port);

/// @brief Read a word from an io port.
/// @param port Port to read from.
/// @return Value that was read from the port.
uint16_t inw(const uint16_t port);

/// @brief Read a dword from an io port.
/// @param port Port to read from.
/// @return Value that was read from the port.
uint32_t inl(const uint16_t port);

/// @brief Read a word x times from an io port.
/// @param port Port to read from.
/// @param data Data to read to.
/// @param size Amount of words to read.
void insm(const uint16_t port, uint8_t *data, uint32_t size);

/// @brief Wait a couple of microseconds.
#define __IO_WAIT() { asm volatile("outb %%al, $0x80" : : "a"(0)); }

/// @brief Disable interrupts.
#define __CLI()     { asm volatile("cli"); }

/// @brief Enable interrupts.
#define __STI()     { asm volatile("sti"); }

/// @brief Halt the processor.
#define __HALT()    { asm volatile("hlt"); }

/// @brief Do nothing.
#define __PAUSE()   { asm volatile("pause"); }

/// @brief Halt infinitely the CPU.
#define __HCF() { \
    __CLI();        \
    while (1)       \
        __HALT();   \
}
