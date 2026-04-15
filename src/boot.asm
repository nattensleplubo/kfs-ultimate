bits 32

section .multiboot
align 4
	dd 0x1BADB002			
	dd 0x00					
	dd -(0x1BADB002 + 0x00)	

section .text
extern kernel_main
global _start
global halt
global gdt_flush

halt:
	hlt

_start:
	mov		esp, stack_top	
	push	ebx				
	call	kernel_main
	call	halt

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]
    jmp 0x08:.flush
.flush:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

section .bss
align 16
stack_space:
	resb	16384
stack_top:
