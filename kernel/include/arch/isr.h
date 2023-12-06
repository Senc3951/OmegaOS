#pragma once

#include <arch/idt.h>

/// @brief Stack when receiving the interrupt.
typedef struct INTERRUPT_STACK
{
    uint64_t ds;
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t cr0, cr2, cr3;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t interruptNumber, errorCode;
    uint64_t rip, cs, rflags, rsp, ss;
} __PACKED__ InterruptStack_t;

/// @brief List of available interrupts.
enum InterruptsList
{
    DivisionError  = 0,
    DebugError,
    NMIError,
    BreakpointError,
    OverflowError,
    BoundRangeError,
    InvalidOpcodeError,
    DeviceNoAvailableError,
    DoubleFaultError,
    CoprocessorSegmentOverrunError,
    InvalidTSSError,
    SegmentNotPresent,
    StackSegmentFault,
    GeneralProtectionFault,
    PageFault,
    x87FloatingPointError = 16,
    AlignmentCheckError,
    MachineCheckError,
    SIMDFloatingPointError,
    VirtualizationError,
    ControlProtectionError,
    HypervisorInjectionError = 28,
    VMMCommunicationError,
    SecurityError,
};

typedef void (*ISRHandler)(InterruptStack_t *stack);

/// @brief Initialize the ISR routines.
void isr_init();

/// @brief Register an interrupt handler.
/// @param interrupt Interrupt number to register the handler to.
/// @param autoUnmaskIRQ Unmask the IRQ line if enabled.
/// @param handler Handler that will be called on the interrupt.
/// @return True if successfully registered the interrupt, False, otherwise.
bool isr_registerHandler(const uint8_t interrupt, const bool autoUnmaskIRQ, ISRHandler handler);