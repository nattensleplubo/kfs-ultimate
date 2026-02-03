.set MAGIC, 0x1BADB002
.set FLAGS, (1<<0 | 1<<1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
	.long MAGIC
	.long FLAGS
	.long CHECKSUM

.section .text
.extern kernel_main
.global _start

_start:
	mov $stack_top, %esp
	call kernel_main
	cli
hang:
	hlt
	jmp hang

.section .bss
.space 16384
stack_top:
