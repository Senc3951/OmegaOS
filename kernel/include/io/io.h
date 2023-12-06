#pragma once

#include <common.h>

/**
 * @brief Write a Byte to an IO port.
 * 
 * @param port Port to write the value to.
 * @param value Value to write.
*/
void outb(const uint16_t port, const uint8_t value);

/**
 * @brief Write a Word to an IO port.
 * 
 * @param port Port to write the value to.
 * @param value value to wrote.
*/
void outw(const uint16_t port, const uint16_t value);

/**
 * @brief Write a DWord to an IO port.
 * 
 * @param port Port to write the value to.
 * @param value value to wrote.
*/
void outl(const uint16_t port, const uint32_t value);

/**
 * @brief Write a Byte size times.
 * 
 * @param port Port to write to.
 * @param data Array to write.
 * @param size Count to write.
*/
void outsm(const uint16_t port, uint8_t *data, uint32_t size);

/**
 * @brief Read a Byte from an IO port.
 * 
 * @param port Port to read from.
 * @return Value that was read from the port.
*/
uint8_t inb(const uint16_t port);

/**
 * @brief Read a Word from an IO port.
 * 
 * @param port Port to read from.
 * @return Value that was read from the port.
*/
uint16_t inw(const uint16_t port);

/**
 * @brief Read a DWord from an IO port.
 * 
 * @param port Port to read from.
 * @return Value that was read from the port.
*/
uint32_t inl(const uint16_t port);

/**
 * @brief Read a Byte size times.
 * 
 * @param port Port to read from.
 * @param data Array to read to.
 * @param size Count to read.
*/
void insm(const uint16_t port, uint8_t *data, uint32_t size);

/**
 * @brief Wait a couple of microseconds.
*/
#define __IO_WAIT() { asm volatile("outb %%al, $0x80" : : "a"(0)); }

/**
 * @brief Disable interrupts.
*/
#define __CLI() { asm volatile("cli"); }

/**
 * @brief Enable interrupts.
*/
#define __STI() { asm volatile("sti"); }

/**
 * @brief Halt the processor.
*/
#define __HALT() { asm volatile("hlt"); }

/**
 * @brief Halt infinitely the CPU.
*/
#define __HCF() { \
    __CLI();        \
    while (1)       \
        __HALT();   \
}
