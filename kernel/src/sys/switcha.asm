bits 64

global x64_switch_processes
x64_switch_processes:  ; rdi - cs, rsi - ds, rdx - PRegisters_t*
    cli
    
    xor rax, rax
    mov ax, si
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push rax                    ; ss
    push qword [rdx]            ; rsp
    pushfq                      ; rflags
    
    ; enable interrupts
    pop rax
    or rax, 0x200               ; set the IF flag
    push rax
    
    push rdi                    ; cs
    push qword [rdx + 0x10]     ; rip
    
    ; load basic registers
    mov rbp, [rdx + 0x8]
    mov rax, [rdx + 0x20]
    mov rbx, [rdx + 0x28]
    mov rcx, [rdx + 0x30]
    mov rdi, [rdx + 0x40]
    mov rsi, [rdx + 0x48]
    mov rdx, [rdx + 0x38]
    
    iretq