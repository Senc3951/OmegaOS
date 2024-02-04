#pragma once

#include <common.h>

#define ICR_FIXED       0x00000000
#define ICR_LOWEST      0x00000100
#define ICR_SMI         0x00000200
#define ICR_NMI         0x00000400
#define ICR_INIT        0x00000500
#define ICR_STARTUP     0x00000600

/// @brief Send an ipi to a core.
/// @param destID APIC id of the core.
/// @param mode Mode of delivery.
/// @param vector Interrupt to send.
void ipi_send_core(const uint8_t destID, const uint8_t mode, const uint8_t vector);

/// @brief Send an ipi deassert to a core.
/// @param destID APIC id of the core.
/// @param mode Mode of delivery.
/// @param vector Interrupt to send.
void ipi_send_deassert(const uint8_t destID, const uint8_t mode, const uint8_t vector);

/// @brief Send an ipi to all cores.
/// @param mode Mode of delivery.
/// @param vector Interrupt to send.
void ipi_broadcast(const uint8_t mode, const uint8_t vector);

/// @brief Wait for icr accept.
/// @return True if accepted, False, otherwise.
bool ipi_wait_accept();

/// @brief Initialize IPI communication.
void ipi_init();