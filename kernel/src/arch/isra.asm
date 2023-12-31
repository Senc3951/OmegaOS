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

%macro pushaq 0
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rdx
    pop rbx
    pop rax
%endmacro

%include "src/arch/isra.inc"

isr_common:
    cld
    pushaq
        
    xor rax, rax
    mov ax, ds
    push rax
    
    mov ax, KERNEL_DS   ; switch to kernel ds
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rdi, rsp
    call isr_interrupt_handler
    
    pop rax     ; restore ds
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popaq
    add rsp, 16     ; remove interrupt number and error code    
    iretq

interruptHandlers:
    %assign i 0
    %rep 0xFF
        dq INT%+i
    %assign i i+1
    %endrep