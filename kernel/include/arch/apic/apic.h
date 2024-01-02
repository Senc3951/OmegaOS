#pragma once

#include <stdint.h>

enum
{
    LAPIC_REG_EOI                   = 0xB0,
    LAPIC_REG_SPURIOUS_IV           = 0xF0,
    LAPIC_REG_INTERRUPT_COMMAND     = 0x300
};

/// @brief Initialize the APIC.
void apic_init();

/// @brief Read a register of the local apic.
/// @param offset Offset of the register.
/// @return Value of the register.
uint32_t apic_read_register(const uint32_t offset);

/// @brief Write a value to a register of the local apic.
/// @param offset Offset of the register.
/// @param value Value to write to the register
void apic_write_register(const uint32_t offset, const uint32_t value);

/// @brief Send an eoi to the apic.
void apic_send_eoi();