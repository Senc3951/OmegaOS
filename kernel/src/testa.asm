bits 64

global x64_test
x64_test:
    mov rax, 1
    mov rdi, 1
    mov rsi, 2
    mov rdx, 3
    mov rcx, 4
    mov rbx, 5
    int 0x80
    
    xor rax, rax
    int 0x80