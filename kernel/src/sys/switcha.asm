bits 64

global x64_switch_processes
x64_switch_processes:  ; rdi - Context_t*, rsi - cr3
    cli
    
    mov cr3, rsi                ; switch to process pml4
    
    xor rax, rax
    mov ax, [rdi + 0x20]        ; ds
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push rax                    ; ss
    push qword [rdi + 0x10]     ; rsp
    push qword [rdi + 0x18]     ; rflags
    
    ; make sure interrupts are enabled
    pop rax
    or rax, 0x200               ; set the IF flag
    push rax
    
    push qword [rdi + 0x8]      ; cs
    push qword [rdi]            ; rip
    
    ; load basic registers
    mov rax, [rdi + 0x28]
    mov rbx, [rdi + 0x30]
    mov rcx, [rdi + 0x38]
    mov rdx, [rdi + 0x40]
    mov rsi, [rdi + 0x50]
    mov rbp, [rdi + 0x58]

    mov r8, [rdi + 0x60]
    mov r9, [rdi + 0x68]
    mov r10, [rdi + 0x70]
    mov r11, [rdi + 0x78]
    mov r12, [rdi + 0x80]
    mov r13, [rdi + 0x88]
    mov r14, [rdi + 0x90]
    mov r15, [rdi + 0x98]
    mov rdi, [rdi + 0x48]
    
    iretq