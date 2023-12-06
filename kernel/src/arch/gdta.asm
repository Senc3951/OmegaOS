bits 64

global x64_load_gdt
x64_load_gdt:
    lgdt [rdi]
    
    push rsi    ; Reload CS
    lea rax, [rel .reload_ds]
    push rax
    retfq
.reload_ds:
    mov ax, dx
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret