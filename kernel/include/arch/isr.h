#pragma once

#include <arch/idt.h>

/// @brief Stack when receiving the interrupt.
typedef struct INTERRUPT_STACK
{
    uint64_t ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t interruptNumber, errorCode;
    uint64_t rip, cs, rflags, rsp, ss;
} __PACKED__ InterruptStack_t;

/// @brief List of available interrupts.
enum INTERRUPT_LIST
{
    DivisionError = 0,
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

enum IRQ_INTERRUPT_LIST
{
    IRQ_PIT         = 32 + 0,
    IRQ_KEYBOARD    = 32 + 1
};

typedef void (*ISRHandler)(InterruptStack_t *stack);

/// @brief Initialize the ISR routines.
void isr_init();

/// @brief Register an interrupt handler.
/// @param interrupt Interrupt number to register the handler to.
/// @param handler Handler that will be called on the interrupt.
/// @return True if successfully registered the interrupt, False, otherwise.
bool isr_registerHandler(const uint8_t interrupt, ISRHandler handler);