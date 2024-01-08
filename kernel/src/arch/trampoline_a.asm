default abs

%define	OFFSET_REL(abs)	(abs - $$)

%define	GDT_CODE_SEG	(trampoline_gdt_start.code - trampoline_gdt_start)
%define	GDT_DATA_SEG	(trampoline_gdt_start.data - trampoline_gdt_start)
%define	GDT_SYSTEM_SEG	(trampoline_gdt_start.system - trampoline_gdt_start)

bits 16

global trampoline_code
trampoline_code:
	cli
	cld
	
	mov bx, cs
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	shl ebx, 4 ; needs base address for linear space

	mov byte [ds:OFFSET_REL(trampoline_data.ap_status)], 1	; tell the BSP the core started
.spin:
	cmp	byte[ds:OFFSET_REL(trampoline_data.bsp_status)], 0
	je short .spin
.continue:
	mov byte [ds:OFFSET_REL(trampoline_data.ap_status)], 0	; reset status

	; figure out the run-time address of the gdt and load it
	mov	eax, ebx
	add	eax, OFFSET_REL(trampoline_gdt_start)
	mov	dword [ds:OFFSET_REL(trampoline_gdt_descriptor + 2)], eax
	lgdt [ds:OFFSET_REL(trampoline_gdt_descriptor)]

	; enable protected mode
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	; enter protected mode
	mov eax, ebx
	add eax, OFFSET_REL(trampoline_protected)
	mov dword [OFFSET_REL(trampoline_spring)], eax
	jmp far dword [OFFSET_REL(trampoline_spring)]

bits 32
trampoline_protected:
	mov ax, GDT_DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; enable PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	; enable long mode
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; load page table
	mov eax, dword [ebx + OFFSET_REL(trampoline_data.pml4)]
	mov cr3, eax

	; enable paging
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	; enter long mode, reusing the old spring
	lea eax, [ebx + OFFSET_REL(trampoline_longmode)]
	mov dword [ebx + OFFSET_REL(trampoline_spring)], eax
	mov word [ebx + OFFSET_REL(trampoline_spring) + 4], GDT_SYSTEM_SEG
	jmp far dword [ebx + OFFSET_REL(trampoline_spring)]

bits 64
trampoline_longmode:
	mov ax, 0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; enable APIC
	mov ecx, 0x1b
	rdmsr
	or eax, 0x800
	wrmsr

	; setup stack
	mov rsp, qword [ebx + OFFSET_REL(trampoline_data.stack_top)]
	mov rbp, 0	; null base pointer for stack unwinding

	; signal the bsp the code finished initializing
    mov byte [ebx + OFFSET_REL(trampoline_data.ap_status)], 1

	mov rdi, [ebx + OFFSET_REL(trampoline_data.core_context)]
	call qword [ebx + OFFSET_REL(trampoline_data.ap_entry)]

align 64
global trampoline_data
trampoline_data:
	.ap_status:		db 0
	.bsp_status:	db 0
	.pml4:			dd 0
	.stack_top:		dq 0
	.ap_entry:		dq 0
	.core_context:	dq 0

align 16
trampoline_gdt_start:
.null:
	dq 0
.code:
	dw 0xffff
	dw 0x0000
	db 0x00
	db 0x9a
	db 0xcf
	db 0x00
.data:
	dw 0xffff
	dw 0x0000
	db 0x00
	db 0x92
	db 0xcf
	db 0x00
.system:
	dw 0x0000
	dw 0x0000
	db 0x00
	db 0x98
	db 0xa0
	db 0x00
trampoline_gdt_end:

align 4
trampoline_gdt_descriptor:
	dw trampoline_gdt_end - trampoline_gdt_start - 1
	dd 00	; figure out at runtime

align 4
trampoline_spring:
	dd 0
	dw GDT_CODE_SEG

times 4096 - ($ - $$) db 0