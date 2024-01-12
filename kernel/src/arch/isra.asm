bits 64

%define KERNEL_DS   0x10
%define SYSCALL_INT 0x80

extern isr_interrupt_handler, _KernelPML4
global interruptHandlers

%macro ISR_EXCEPTION 2
    global INT%1
    INT%1:
        %if %2 == 1
            push qword 0    ; push dummy error code
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
%endmacro

%include "src/arch/isra.inc"

isr_common:
    cld
    pushaq

    ; handle inside kernel page table
    mov rax, [_KernelPML4]
    mov cr3, rax

    ; store current segments
    xor rax, rax
    mov ax, ds
    push rax
    
    ; handle inside kernel segments
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rdi, rsp
    call isr_interrupt_handler
    
    ; restore saved segments
    pop rbx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    
    ; restore registers
    popaq
    
    ; check if should restore rax or not
    cmp byte [rsp + 8], SYSCALL_INT
    je .syscall
    pop rax         ; restore
    jmp .continue
.syscall:
    add rsp, 8      ; don't overwrite the return value
.continue:
    add rsp, 16     ; remove interrupt number and error code    
    iretq

interruptHandlers:
    %assign i 0
    %rep 0xFF
        dq INT%+i
    %assign i i+1
    %endrep