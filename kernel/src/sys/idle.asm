bits 64

global x64_idle
x64_idle:
.loop:
    hlt
    jmp .loop