OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_entry)

SECTIONS
{
    . = 2M;
    _kernel_start = .;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }
    
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }
    
    _kernel_writable_start = .;
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }
    
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
    _kernel_writable_end = .;
    
    /DISCARD/ :
    {
        *(.eh_frame)
        *(.note)
    }
    
    _kernel_end = .;
}