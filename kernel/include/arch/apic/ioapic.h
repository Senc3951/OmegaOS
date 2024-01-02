#pragma once

enum
{
    IOAPIC_REG_REDIRECTION_ENTRIES = 1
};

/// @brief Initialize the IOAPIC.
void ioapic_init();