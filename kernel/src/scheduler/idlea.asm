bits 64

global x64_idle_proccess
x64_idle_proccess:
.loop:
    sti
    hlt
    jmp .loop