bits 64

global t1, t2
t1:
    mov rax, 1
    mov rdi, 1
    int 0x80
    mov rax, 0
    int 0x80

t2:
    mov rax, 1
    mov rdi, 2
.loop:
    int 0x80
    jmp .loop