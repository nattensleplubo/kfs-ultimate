AS = as --32
CC = gcc -m32
LD = ld -m elf_i386

CFLAGS = -std=c99 -ffreestanding -O2 -Wall -Wextra \
	 -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs

OBJS = boot.o kernel.o k_lib.o

kernel.bin: $(OBJS)
	$(LD) -T linker.ld -o kernel.bin $(OBJS) -nostdlib

boot.o: boot.s
	$(AS) boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

k_lib.o: k_lib.c
	$(CC) $(CFLAGS) -c k_lib.c -o k_lib.o

# vga.o: vga.c
# 	$(CC) $(CFLAGS) -c vga.c -o vga.o

iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	echo 'menuentry "My Kernel" {' > isodir/boot/grub/grub.cfg
	echo '	multiboot /boot/kernel.bin' >> isodir/boot/grub/grub.cfg
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
