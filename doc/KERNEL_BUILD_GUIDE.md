# Building a Bootable Kernel - Complete Guide (KFS-1)

A step-by-step guide to cross-compiling, linking, and creating a bootable ISO for your kernel project.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Cross-Compiler Setup](#cross-compiler-setup)
4. [Project Structure](#project-structure)
5. [The Multiboot Header](#the-multiboot-header)
6. [Linker Script](#linker-script)
7. [Makefile](#makefile)
8. [Building the Kernel](#building-the-kernel)
9. [Creating a Bootable ISO](#creating-a-bootable-iso)
10. [Testing with QEMU](#testing-with-qemu)
11. [Troubleshooting](#troubleshooting)

---

## Overview

### What You're Building

```
Source Files (C + ASM)
        ↓
    Compile to Object Files (.o)
        ↓
    Link into Single Binary (kernel.bin)
        ↓
    Package into ISO with GRUB
        ↓
    Boot in QEMU/VirtualBox/Real Hardware
```

### Why Cross-Compilation?

Your **host system** (Linux/macOS) has a compiler that targets your OS and architecture. But you're building a **freestanding binary** (no OS) for **i686** (32-bit x86).

**Cross-compiler** = compiler that runs on your system but generates code for a different target.

---

## Prerequisites

### Tools You Need

```bash
# Ubuntu/Debian
sudo apt-get install build-essential nasm qemu-system-x86 \
    xorriso grub-pc-bin grub-common mtools

# Arch Linux
sudo pacman -S base-devel nasm qemu xorriso grub mtools

# macOS (with Homebrew)
brew install nasm qemu xorriso grub i686-elf-gcc
```

### What Each Tool Does

- **gcc/clang**: C compiler
- **nasm**: Assembler (converts .asm → .o)
- **ld**: Linker (combines .o files → binary)
- **grub-mkrescue**: Creates bootable ISO
- **xorriso**: ISO creation utility
- **qemu**: Virtual machine for testing
- **mtools**: File manipulation for ISO

---

## Cross-Compiler Setup

### Option 1: Use System Compiler with Flags (Quick & Easy)

For KFS-1, you can usually use your system's GCC with the right flags:

```bash
# Check if your GCC can target i686
gcc -m32 --version
```

If this works, you can use:
```bash
gcc -m32 -ffreestanding ...
```

### Option 2: Build a Proper Cross-Compiler (Recommended)

A true i686-elf cross-compiler is cleaner and avoids host system contamination.

#### Step 1: Download Binutils and GCC

```bash
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p ~/cross-compiler
cd ~/cross-compiler

# Download
wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz

tar -xf binutils-2.41.tar.xz
tar -xf gcc-13.2.0.tar.xz
```

#### Step 2: Build Binutils

```bash
mkdir build-binutils
cd build-binutils

../binutils-2.41/configure --target=$TARGET --prefix="$PREFIX" \
    --with-sysroot --disable-nls --disable-werror

make -j$(nproc)
make install

cd ..
```

#### Step 3: Build GCC

```bash
mkdir build-gcc
cd build-gcc

../gcc-13.2.0/configure --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c,c++ --without-headers

make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc
make install-target-libgcc

cd ..
```

#### Step 4: Verify Installation

```bash
$HOME/opt/cross/bin/i686-elf-gcc --version
$HOME/opt/cross/bin/i686-elf-ld --version
```

#### Step 5: Add to PATH (Permanent)

Add to `~/.bashrc` or `~/.zshrc`:
```bash
export PATH="$HOME/opt/cross/bin:$PATH"
```

---

## Project Structure

```
kfs-1/
├── Makefile
├── linker.ld              # Linker script
├── grub.cfg               # GRUB configuration
├── src/
│   ├── boot/
│   │   └── boot.asm       # Multiboot header + entry point
│   ├── kernel/
│   │   ├── kernel.c       # Main kernel code
│   │   ├── gdt.c          # GDT implementation
│   │   ├── idt.c          # IDT implementation
│   │   └── vga.c          # VGA text mode
│   └── include/
│       ├── kernel.h
│       ├── gdt.h
│       ├── idt.h
│       └── vga.h
├── build/                 # Generated .o files
└── iso/                   # ISO build directory
    └── boot/
        └── grub/
            └── grub.cfg
```

---

## The Multiboot Header

GRUB needs a **Multiboot header** in your kernel binary to recognize it as bootable.

### boot.asm

```asm
; boot.asm - Multiboot header and kernel entry point

MBOOT_MAGIC     equ 0x1BADB002          ; Multiboot magic number
MBOOT_FLAGS     equ (1 << 0) | (1 << 1) ; Page align + memory info
MBOOT_CHECKSUM  equ -(MBOOT_MAGIC + MBOOT_FLAGS)

[BITS 32]                               ; 32-bit protected mode

section .multiboot
align 4
    dd MBOOT_MAGIC                      ; Magic number
    dd MBOOT_FLAGS                      ; Flags
    dd MBOOT_CHECKSUM                   ; Checksum

section .bss
align 16
stack_bottom:
    resb 16384                          ; 16 KiB stack
stack_top:

section .text
global _start                           ; Entry point
extern kernel_main                      ; C function

_start:
    ; Set up stack
    mov esp, stack_top
    mov ebp, esp
    
    ; Reset EFLAGS
    push 0
    popf
    
    ; Call C kernel
    call kernel_main
    
    ; If kernel returns, halt
    cli
.hang:
    hlt
    jmp .hang

; You can add other assembly functions here
global gdt_flush
extern gdt_ptr

gdt_flush:
    lgdt [gdt_ptr]                      ; Load GDT
    mov ax, 0x10                        ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush                     ; Far jump to code segment
.flush:
    ret

global idt_flush
extern idt_ptr

idt_flush:
    lidt [idt_ptr]                      ; Load IDT
    ret
```

### kernel.c

```c
// kernel.c - Main kernel entry point

#include <stdint.h>
#include <stddef.h>

// VGA text mode buffer
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static size_t vga_row = 0;
static size_t vga_col = 0;

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
        return;
    }
    
    size_t index = vga_row * VGA_WIDTH + vga_col;
    // White on black: 0x0F (foreground) on 0x00 (background)
    vga_buffer[index] = ((uint16_t)0x0F << 8) | c;
    
    vga_col++;
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
}

void vga_print(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

void kernel_main(void) {
    // Clear screen
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ((uint16_t)0x0F << 8) | ' ';
    }
    
    vga_print("Hello from KFS-1!\n");
    vga_print("Kernel is running...\n");
    
    // Infinite loop
    while (1) {
        __asm__ volatile ("hlt");
    }
}
```

---

## Linker Script

The **linker script** tells the linker how to organize your kernel in memory.

### linker.ld

```ld
/* linker.ld - Linker script for kernel */

ENTRY(_start)                   /* Entry point symbol */

SECTIONS
{
    /* Kernel starts at 1MB (0x100000) */
    . = 1M;
    
    _kernel_start = .;
    
    /* Multiboot header must be in first 8KB */
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }
    
    /* Code section */
    .text ALIGN(4K) : {
        *(.text)
    }
    
    /* Read-only data */
    .rodata ALIGN(4K) : {
        *(.rodata)
    }
    
    /* Initialized data */
    .data ALIGN(4K) : {
        *(.data)
    }
    
    /* Uninitialized data */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }
    
    _kernel_end = .;
}
```

**Key points:**
- `. = 1M` places kernel at 1MB (GRUB loads it there)
- `ALIGN(4K)` aligns sections to 4KB boundaries (page size)
- `.multiboot` section must come first for GRUB to find it
- `_kernel_start` and `_kernel_end` are symbols you can use in C

---

## Makefile

### Complete Makefile

```makefile
# Makefile for KFS-1

# Compiler and tools
ASM := nasm
CC := i686-elf-gcc
LD := i686-elf-ld

# Alternative: use system GCC with -m32
# CC := gcc -m32
# LD := ld -m elf_i386

# Flags
ASMFLAGS := -f elf32
CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror \
          -nostdlib -nostartfiles -nodefaultlibs \
          -Isrc/include
LDFLAGS := -T linker.ld -nostdlib

# Directories
SRC_DIR := src
BUILD_DIR := build
ISO_DIR := iso
BOOT_DIR := $(ISO_DIR)/boot
GRUB_DIR := $(BOOT_DIR)/grub

# Source files
ASM_SRC := $(shell find $(SRC_DIR) -name '*.asm')
C_SRC := $(shell find $(SRC_DIR) -name '*.c')

# Object files
ASM_OBJ := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SRC))
C_OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRC))
OBJ := $(ASM_OBJ) $(C_OBJ)

# Output
KERNEL := $(BOOT_DIR)/kernel.bin
ISO := kfs-1.iso

# Colors for pretty output
GREEN := \033[0;32m
YELLOW := \033[0;33m
NC := \033[0m

.PHONY: all clean iso run

all: $(KERNEL)

# Create directories
$(BUILD_DIR) $(BOOT_DIR) $(GRUB_DIR):
	@mkdir -p $@

# Assemble .asm files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)[ASM]$(NC) $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

# Compile .c files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)[CC]$(NC) $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(KERNEL): $(OBJ) | $(BOOT_DIR)
	@echo "$(GREEN)[LD]$(NC) $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Create GRUB config
$(GRUB_DIR)/grub.cfg: | $(GRUB_DIR)
	@echo "menuentry \"KFS-1\" {" > $@
	@echo "    multiboot /boot/kernel.bin" >> $@
	@echo "}" >> $@

# Create ISO
iso: $(ISO)

$(ISO): $(KERNEL) $(GRUB_DIR)/grub.cfg
	@echo "$(GREEN)[ISO]$(NC) $@"
	@grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null

# Run in QEMU
run: $(ISO)
	@echo "$(GREEN)[QEMU]$(NC) Running kernel..."
	@qemu-system-i386 -cdrom $(ISO)

# Run in QEMU with serial output
run-serial: $(ISO)
	@qemu-system-i386 -cdrom $(ISO) -serial stdio

# Debug with GDB
debug: $(ISO)
	@qemu-system-i386 -cdrom $(ISO) -s -S

# Clean build artifacts
clean:
	@echo "$(YELLOW)[CLEAN]$(NC)"
	@rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO)

# Print variables (for debugging Makefile)
print-%:
	@echo $* = $($*)
```

### Makefile Explanation

**Variables:**
- `ASM`, `CC`, `LD`: Tools to use
- `ASMFLAGS`: NASM flags (`-f elf32` = 32-bit ELF format)
- `CFLAGS`: 
  - `-ffreestanding`: No standard library
  - `-O2`: Optimization level 2
  - `-nostdlib`: Don't link standard library
  - `-Isrc/include`: Include directory
- `LDFLAGS`: `-T linker.ld` uses our linker script

**Targets:**
- `all`: Build kernel binary
- `iso`: Create bootable ISO
- `run`: Boot in QEMU
- `clean`: Remove build artifacts

**Pattern Rules:**
- `$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm`: Assemble each .asm file
- `$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c`: Compile each .c file

---

## Building the Kernel

### Step-by-Step Build Process

```bash
# 1. Clean previous builds
make clean

# 2. Build kernel binary
make

# 3. Create bootable ISO
make iso

# 4. Run in QEMU
make run
```

### What Happens Under the Hood

```
1. Assemble boot.asm
   nasm -f elf32 src/boot/boot.asm -o build/boot/boot.o

2. Compile kernel.c
   i686-elf-gcc -ffreestanding -c src/kernel/kernel.c -o build/kernel/kernel.o

3. Link all .o files
   i686-elf-ld -T linker.ld -o iso/boot/kernel.bin build/boot/boot.o build/kernel/kernel.o

4. Create ISO with GRUB
   grub-mkrescue -o kfs-1.iso iso/
```

---

## Creating a Bootable ISO

### GRUB Configuration

Create `grub.cfg` (Makefile does this automatically):

```
menuentry "KFS-1" {
    multiboot /boot/kernel.bin
}
```

### Manual ISO Creation

If not using Makefile:

```bash
# Create directory structure
mkdir -p iso/boot/grub

# Copy kernel
cp kernel.bin iso/boot/

# Create grub.cfg
cat > iso/boot/grub/grub.cfg << EOF
menuentry "KFS-1" {
    multiboot /boot/kernel.bin
}
EOF

# Create ISO
grub-mkrescue -o kfs-1.iso iso/
```

### Verifying the ISO

```bash
# Check if it's bootable
file kfs-1.iso

# Output should mention "bootable"
```

---

## Testing with QEMU

### Basic Run

```bash
qemu-system-i386 -cdrom kfs-1.iso
```

### Useful QEMU Options

```bash
# More memory
qemu-system-i386 -cdrom kfs-1.iso -m 512M

# Serial output (for debugging)
qemu-system-i386 -cdrom kfs-1.iso -serial stdio

# No graphics (serial only)
qemu-system-i386 -cdrom kfs-1.iso -nographic -serial stdio

# Monitor mode (for debugging)
qemu-system-i386 -cdrom kfs-1.iso -monitor stdio

# Debug with GDB
qemu-system-i386 -cdrom kfs-1.iso -s -S
# Then in another terminal: gdb kernel.bin
# (gdb) target remote localhost:1234
```

### Testing on Real Hardware

```bash
# Write to USB (BE CAREFUL - double check device name!)
sudo dd if=kfs-1.iso of=/dev/sdX bs=4M status=progress
sudo sync

# Or burn to CD
```

---

## Troubleshooting

### Common Errors and Solutions

#### 1. "No multiboot header found"

**Problem**: GRUB can't find the multiboot header.

**Solutions:**
- Ensure `.multiboot` section is **first** in linker script
- Verify boot.asm has correct multiboot magic number (0x1BADB002)
- Check that boot.o is linked **first**: `ld ... boot.o kernel.o ...`

```bash
# Check if multiboot header is in binary
hexdump -C iso/boot/kernel.bin | head -20
# Should see: 02 b0 ad 1b somewhere in first 8KB
```

#### 2. "Triple Fault" / QEMU Resets

**Problem**: Kernel crashes immediately.

**Solutions:**
- Check stack is set up correctly in boot.asm (`mov esp, stack_top`)
- Verify your C code doesn't call standard library functions
- Ensure interrupts are disabled (`cli`) if you haven't set up IDT yet

#### 3. Linker Errors

**Problem**: `undefined reference to ...`

**Solutions:**
```bash
# Missing function in C
# → Implement it or remove the call

# Standard library function
# → You can't use stdlib in freestanding environment
# → Implement your own (memcpy, strlen, etc.)
```

#### 4. "Cannot find -lgcc"

**Problem**: Linker looking for GCC runtime library.

**Solution:**
```bash
# Remove -lgcc from linker flags, or
# Build with: make all-target-libgcc
```

#### 5. GRUB-mkrescue Not Found

```bash
# Ubuntu/Debian
sudo apt-get install grub-pc-bin grub-common

# Arch
sudo pacman -S grub

# macOS
brew install grub
```

#### 6. Cross-Compiler Issues

**Problem**: `i686-elf-gcc: command not found`

**Solutions:**
```bash
# Check PATH
echo $PATH

# Verify installation
ls $HOME/opt/cross/bin/

# Use system compiler instead
# In Makefile: CC := gcc -m32, LD := ld -m elf_i386
```

#### 7. Black Screen in QEMU

**Problem**: QEMU boots but screen is black.

**Solutions:**
- Check VGA writes are to correct address (0xB8000)
- Verify you're writing 16-bit values (character + color)
- Try printing to serial instead to debug:

```c
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_putchar(char c) {
    outb(0x3F8, c);  // COM1
}
```

Then run: `qemu-system-i386 -cdrom kfs-1.iso -serial stdio`

---

## Advanced: Debugging with GDB

### Setup

```bash
# Terminal 1: Start QEMU with GDB server
qemu-system-i386 -cdrom kfs-1.iso -s -S

# Terminal 2: Start GDB
i686-elf-gdb iso/boot/kernel.bin

# In GDB:
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
(gdb) layout asm
(gdb) stepi
```

### Useful GDB Commands

```
info registers       # Show all registers
x/20i $eip          # Show next 20 instructions
x/20x $esp          # Show stack contents
break *0x100000     # Break at address
continue            # Resume execution
stepi               # Step one instruction
```

---

## Complete Build Example

Let's put it all together with a minimal working example:

### 1. Project Setup

```bash
mkdir -p kfs-1/{src/boot,src/kernel,src/include}
cd kfs-1
```

### 2. Create Files

Save boot.asm, kernel.c, linker.ld, and Makefile from sections above.

### 3. Build

```bash
make clean
make iso
```

### 4. Expected Output

```
[ASM] src/boot/boot.asm
[CC] src/kernel/kernel.c
[LD] iso/boot/kernel.bin
[ISO] kfs-1.iso
```

### 5. Run

```bash
make run
```

You should see "Hello from KFS-1!" in the QEMU window!

---

## Quick Reference Commands

```bash
# Build everything
make iso

# Clean and rebuild
make clean && make iso

# Run in QEMU
make run

# Run with serial output
make run-serial

# Debug with GDB
make debug
# (in another terminal) i686-elf-gdb iso/boot/kernel.bin

# Check kernel binary
file iso/boot/kernel.bin
hexdump -C iso/boot/kernel.bin | head

# Check ISO
file kfs-1.iso
```

---

## Next Steps

Now that you can build and boot a kernel:

1. **Implement GDT**: Set up Global Descriptor Table
2. **Implement IDT**: Set up Interrupt Descriptor Table  
3. **Keyboard Driver**: Handle keyboard interrupts
4. **Better VGA**: Scrolling, colors, cursor
5. **Memory Management**: Physical and virtual memory

Good luck with KFS-1! The build system is now set up - focus on implementing the kernel features.

---

## Additional Resources

- **OSDev Wiki**: https://wiki.osdev.org/
- **GRUB Manual**: https://www.gnu.org/software/grub/manual/
- **GCC Cross-Compiler**: https://wiki.osdev.org/GCC_Cross-Compiler
- **Multiboot Specification**: https://www.gnu.org/software/grub/manual/multiboot/
