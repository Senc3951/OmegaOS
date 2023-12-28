bits 64

global x64_init_proccess
x64_init_proccess:
.loop:
    hlt
    jmp .loop