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

halt:
	hlt

_start:
	mov		esp, stack_top	
	push	ebx				
	call	kernel_main
	call	halt

section .bss
align 16
stack_space:
	resb	16384
stack_top:
