bits 64

global x64_acquire_lock, x64_release_lock, x64_lock_used, x64_atomic_add

spin_wait:
    pause                   ; tell the CPU we are spinning
    test dword [rdi], 1     ; is the lock free?
    jnz spin_wait           ; no, wait

x64_acquire_lock:
    lock bts dword [rdi], 0 ; attempt to acquire the lock
    jc spin_wait            ; spin if locked 
    ret                     ; lock obtained

x64_release_lock:
    mov dword [rdi], 0
    ret

x64_lock_used:
    mov rax, 0
    
    test dword [rdi], 1
    jnz .used
    ret
.used:
    mov rax, 1
    ret

x64_atomic_add:
    mov rax, [rdi]
    lock xadd [rdi], rsi
    ret