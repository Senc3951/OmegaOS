bits 64

data1: db "hello world1", 0
f1: db "test_file", 0
data2: db "content of file", 0

global t1, t2
t1:
    mov rax, 1
    mov rdi, 1
    mov rsi, data1
    mov rdx, 13
    int 0x80
    
    mov rax, 2
    mov rdi, f1
    mov rsi, 0
    mov rdx ,0
    int 0x80

    mov rdi, rax    ; fd
    mov rax, 1
    mov rsi, data2
    mov rdx, 15
    int 0x80

    ; exit
    mov rax, 4
    int 0x80

t2:
.loop:
    jmp .loop