bits 64

global shell
shell:
    sub rsp, 4
    mov qword [rsp], 5
    
    mov rax, 4
    int 0x80