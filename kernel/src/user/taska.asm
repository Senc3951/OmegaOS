bits 64

global x64_enter_userspace
x64_enter_userspace:
    cli
    
    ; set data segment
    xor rax, rax
    mov ax, di
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push rax        ; SS
    push rsp        ; RSP
    pushfq          ; flags

    ; enable interrupts
    pop rax
    or rax, 0x200   ; set the IF flag
    push rax

    push rsi        ; code segment
    push rdx        ; RIP
    
    iretq