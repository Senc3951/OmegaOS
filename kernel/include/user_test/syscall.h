#pragma once

#include <stdint.h>
#include <stddef.h>

inline void syscall1(uint64_t p, uint64_t param1) {    
    asm volatile (
        "mov %[num], %%rax\n\t"    // Set the syscall number in rax
        "mov %[param1], %%rdi\n\t" // Set the value for rdi
        "int $0x80"                 // Trigger uint64_terrupt 0x80
        :
        : [num] "r" (p), [param1] "r" (param1)
        : "rax", "rdi"
    );
}

inline void syscall2(uint64_t p, uint64_t param1, uint64_t param2) {    
    asm volatile (
        "mov %[num], %%rax\n\t"    // Set the syscall number in rax
        "mov %[param1], %%rdi\n\t" // Set the value for rdi
        "mov %[param2], %%rsi\n\t" // Set the value for rsi
        "int $0x80"                 // Trigger uint64_terrupt 0x80
        :
        : [num] "r" (p), [param1] "r" (param1), [param2] "r" (param2)
        : "rax", "rdi", "rsi"
    );
}

inline void syscall3(uint64_t p, uint64_t param1, uint64_t param2, uint64_t param3) {    
    asm volatile (
        "mov %[num], %%rax\n\t"    // Set the syscall number in rax
        "mov %[param1], %%rdi\n\t" // Set the value for rdi
        "mov %[param2], %%rsi\n\t" // Set the value for rsi
        "mov %[param3], %%rdx\n\t" // Set the value for rdx
        "int $0x80"                 // Trigger uint64_terrupt 0x80
        :
        : [num] "r" (p), [param1] "r" (param1), [param2] "r" (param2), [param3] "r" (param3)
        : "rax", "rdi", "rsi", "rdx"
    );
}