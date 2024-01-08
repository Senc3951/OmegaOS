bits 64

global x64_enable_cpu_features
x64_enable_cpu_features:
    ; check for sse
    mov rax, 1
    cpuid
    test edx, 1 << 25
    jz .no_sse

    ; enable sse
    mov rax, cr0
    and ax, 0xfffb
    or ax, 2
    mov cr0, rax

    mov rax, cr4
    or rax, 3 << 9
    mov cr4, rax
    
    mov rax, 1
    ret
.no_sse:
    mov rax, 0
    ret