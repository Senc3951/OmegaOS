#pragma once

#include <common.h>

#define LAPIC_ID                0x0020  // Local APIC ID
#define LAPIC_VER               0x0030  // Local APIC Version
#define LAPIC_TPR               0x0080  // Task Priority
#define LAPIC_APR               0x0090  // Arbitration Priority
#define LAPIC_PPR               0x00a0  // Processor Priority
#define LAPIC_EOI               0x00b0  // EOI
#define LAPIC_RRD               0x00c0  // Remote Read
#define LAPIC_LDR               0x00d0  // Logical Destination
#define LAPIC_DFR               0x00e0  // Destination Format
#define LAPIC_SVR               0x00f0  // Spurious Interrupt Vector
#define LAPIC_ISR               0x0100  // In-Service (8 registers)
#define LAPIC_TMR               0x0180  // Trigger Mode (8 registers)
#define LAPIC_IRR               0x0200  // Interrupt Request (8 registers)
#define LAPIC_ESR               0x0280  // Error Status
#define LAPIC_ICRLO             0x0300  // Interrupt Command
#define LAPIC_ICRHI             0x0310  // Interrupt Command [63:32]
#define LAPIC_TIMER             0x0320  // LVT Timer
#define LAPIC_THERMAL           0x0330  // LVT Thermal Sensor
#define LAPIC_PERF              0x0340  // LVT Performance Counter
#define LAPIC_LINT0             0x0350  // LVT LINT0
#define LAPIC_LINT1             0x0360  // LVT LINT1
#define LAPIC_ERROR             0x0370  // LVT Error
#define LAPIC_TICR              0x0380  // Initial Count (for Timer)
#define LAPIC_TCCR              0x0390  // Current Count (for Timer)
#define LAPIC_TDCR              0x03e0  // Divide Configuration (for Timer)

#define ICR_FIXED               0x00000000
#define ICR_LOWEST              0x00000100
#define ICR_SMI                 0x00000200
#define ICR_NMI                 0x00000400
#define ICR_INIT                0x00000500
#define ICR_STARTUP             0x00000600

#define ICR_PHYSICAL            0x00000000
#define ICR_LOGICAL             0x00000800

#define ICR_IDLE                0x00000000
#define ICR_SEND_PENDING        0x00001000

#define ICR_DEASSERT            0x00000000
#define ICR_ASSERT              0x00004000

#define ICR_EDGE                0x00000000
#define ICR_LEVEL               0x00008000

#define ICR_NO_SHORTHAND        0x00000000
#define ICR_SELF                0x00040000
#define ICR_ALL_INCLUDING_SELF  0x00080000
#define ICR_ALL_EXCLUDING_SELF  0x000c0000

#define APIC_LVT_RESET          0x10000

#define APIC_TRIGGER_EDGE                   0x0
#define APIC_TRIGGER_LEVEL                  0x1

#define APIC_DEST_SHORTHAND_NONE            0x0
#define APIC_DEST_SHORTHAND_SELF            0x1
#define APIC_DEST_SHORTHAND_ALL_AND_SELF    0x2
#define APIC_DEST_SHORTHAND_ALL_BUT_SELF    0x3

#define APIC_LEVEL_DEASSERT                 0x0
#define APIC_LEVEL_ASSERT                   0x1

typedef enum IPI_DELIVERY_MODE
{
    IPI_NORMAL = 0,
    IPI_LOWEST,
    IPI_SMI,
    IPI_NMI = 4,
    IPI_INIT,
    IPI_SIPI
} IpiDeliveryMode_t;

typedef union APIC_ICR
{
    struct
    {
        uint32_t vector     : 8;
        uint32_t delvMode   : 3;
        uint32_t destMode   : 1;
        uint32_t delvStatus : 1;
        uint32_t reserved0  : 1;
        uint32_t level      : 1;
        uint32_t trigger    : 1;
        uint32_t reserved1  : 2;
        uint32_t destType   : 2;
        uint32_t reserved2  : 12;
    } __PACKED__;
    uint32_t raw;
} InterruptCommandRegister_t;

/// @brief Initialize the APIC.
void apic_init();

/// @brief Get the id of the current cpu.
/// @return Current cpu id.
uint32_t apic_get_id();

/// @brief Send an ipi to all cores.
/// @param mode Mode of delivery.
/// @param vector Interrupt to send.
void apic_broadcast_ipi(IpiDeliveryMode_t mode, const uint8_t vector);

/// @brief Set the APIC registers to a well known state. 
void apic_set_registers();

/// @brief Read a register of the local apic.
/// @param offset Offset of the register.
/// @return Value of the register.
uint32_t apic_read_register(const uint32_t offset);

/// @brief Write a value to a register of the local apic.
/// @param offset Offset of the register.
/// @param value Value to write to the register
void apic_write_register(const uint32_t offset, const uint32_t value);

/// @brief Send an eoi to the apic.
void apic_eoi();

extern bool _ApicInitialized;   /* Is APIC initialized. */
extern uint32_t _BspID;         /* APIC ID of the BSP. */