AS = nasm -f elf32 -g -F dwarf
CC = gcc -m32
LD = ld -m elf_i386
CFLAGS = -std=c99 -ffreestanding -O2 -Wall -Wextra \
     -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs

C_SOURCES = $(wildcard *.c)
C_OBJS = $(C_SOURCES:.c=.o)
OBJS = boot.o $(C_OBJS)

kernel.bin: $(OBJS)
	$(LD) -T linker.ld -o kernel.bin $(OBJS) -nostdlib

boot.o: boot.asm
	$(AS) boot.asm -o boot.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	echo 'menuentry "My Kernel" {' > isodir/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.bin' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o mykernel.iso isodir
	rm -rf isodir

run: kernel.bin
	qemu-system-i386 -kernel kernel.bin

run-iso: docker-iso
	qemu-system-i386 -cdrom mykernel.iso

clean:
	rm -f *.o kernel.bin mykernel.iso
	rm -rf isodir

docker-build:
	docker-compose build

docker-iso:
	docker-compose run --rm kernel-build make iso

docker-run:
	docker-compose run --rm kernel-build make run-iso

docker-shell:
	docker-compose run --rm kernel-build bash

docker-clean:
	docker-compose run --rm kernel-build make clean

.PHONY: docker-build docker-iso docker-run docker-shell docker-clean clean run run-iso iso