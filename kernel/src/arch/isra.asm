bits 64

KERNEL_DS equ 0x10

extern isr_interrupt_handler
global interruptHandlers

%macro ISR_EXCEPTION 2
    global INT%1
    INT%1:
        %if %2 == 1
            push qword 0    ; Push dummy error code
        %endif
        
        push qword %1
        jmp isr_common
%endmacro

%macro pusha 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    mov r8, cr3
    push r8
    mov r8, cr2
    push r8
    mov r8, cr0
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
%endmacro

%macro popa 0
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    add rsp, 24
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
%endmacro

%include "src/arch/isra.inc"

isr_common:
    cld
    pusha
    
    xor rax, rax
    mov ax, ds
    push rax
    
    ; Handle the interrupt inside kernels data segment
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rdi, rsp
    call isr_interrupt_handler
    
    ; Restore ds
    pop rax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa
    add rsp, 16     ; Remove interrupt number and error code    
    iretq

interruptHandlers:
    %assign i 0
    %rep 0x2F
        dq INT%+i
    %assign i i+1
    %endrep