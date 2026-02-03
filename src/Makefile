AS = as --32
CC = gcc -m32
LD = ld -m elf_i386

CFLAGS = -std=c99 -ffreestanding -O2 -Wall -Wextra

kernel.bin: boot.o kernel.o
	$(LD) -T linker.ld -o kernel.bin boot.o kernel.o

boot.o: boot.s
	$(AS) boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

run: kernel.bin
	qemu-system-i386 -kernel kernel.bin

clean:
	rm -f *.o kernel.bin
