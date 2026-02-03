nasm boot.asm -f bin -o boot.bin
qemu-system-i386 -fda boot.bin
