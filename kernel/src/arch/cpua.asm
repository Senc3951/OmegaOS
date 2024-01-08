bits 64

global x64_enable_cpu_features
x64_enable_cpu_features:
    ; enable coprocessors (FPU and SSE)
    mov rax, cr0
    and rax, 0xfffffffffffffffb	; disable FPU emulation
    or rax, 0x22				; enable monitoring coprocessor and numeric error
    mov cr0, rax
    mov rax, cr4

    or rax, 0x0406b0			; enable OSFXSR, OSXMMEXCPT and others
    mov cr4, rax	
    fninit

    ret