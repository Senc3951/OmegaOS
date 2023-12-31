#pragma once

#include <stdint.h>

enum
{
    LAPIC_ID_REGISTER               = 0x20,
    LAPIC_TASK_PRIORITY_REGISTER    = 0x80,
    LAPIC_REG_EOI                   = 0xB0,
    LAPIC_REG_SPURIOUS_IV           = 0xF0,
    LAPIC_ERR_STATUS_REGISTER       = 0x280,
    LAPIC_REG_INTERRUPT_COMMAND0    = 0x300,
    LAPIC_REG_INTERRUPT_COMMAND1    = 0x310
};

/// @brief Initialize the APIC.
void apic_init();

/// @brief Initialize the APIC for AP core.
void apic_ap_init();

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