bits 64

global x64_acquire_lock, x64_release_lock

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