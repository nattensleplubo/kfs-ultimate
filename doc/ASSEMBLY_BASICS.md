# Assembly Basics for Kernel Development (KFS-1)

A comprehensive guide to x86 assembly fundamentals for building a simple kernel in C.

## Table of Contents

1. [Introduction to Assembly](#introduction-to-assembly)
2. [CPU Registers](#cpu-registers)
3. [Memory Addressing](#memory-addressing)
4. [Basic Instructions](#basic-instructions)
5. [Control Flow](#control-flow)
6. [Stack Operations](#stack-operations)
7. [Function Calling Conventions](#function-calling-conventions)
8. [Interfacing with C](#interfacing-with-c)
9. [Common Patterns in Kernel Code](#common-patterns-in-kernel-code)

---

## Introduction to Assembly

Assembly language is a low-level programming language that provides a human-readable representation of machine code. Each assembly instruction typically corresponds to one machine instruction that the CPU can execute directly.

### Why Assembly in Kernel Development?

- **Hardware Initialization**: Some CPU operations can only be performed in assembly
- **Boot Process**: The bootloader and early kernel initialization require assembly
- **Performance**: Critical sections can be optimized at the instruction level
- **Direct Hardware Control**: Access to special CPU features and registers

### Syntax Flavors

There are two main assembly syntaxes:

- **Intel Syntax**: `mov destination, source` (more intuitive, used in this guide)
- **AT&T Syntax**: `mov source, destination` (common in GNU tools with `%` for registers)

---

## CPU Registers

Registers are small, extremely fast storage locations built directly into the CPU. Think of them as the CPU's working memory.

### General Purpose Registers (32-bit)

In 32-bit x86 architecture (IA-32), you have 8 main general-purpose registers:

| Register | Name | Primary Use | Preserved Across Calls? |
|----------|------|-------------|-------------------------|
| **EAX** | Accumulator | Math operations, return values | No (caller-saved) |
| **EBX** | Base | General purpose, base pointer | Yes (callee-saved) |
| **ECX** | Counter | Loop counter, string operations | No (caller-saved) |
| **EDX** | Data | Math operations, I/O operations | No (caller-saved) |
| **ESI** | Source Index | String/memory operations (source) | Yes (callee-saved) |
| **EDI** | Destination Index | String/memory operations (dest) | Yes (callee-saved) |
| **EBP** | Base Pointer | Stack frame base pointer | Yes (callee-saved) |
| **ESP** | Stack Pointer | Points to top of stack | Yes (special) |

#### Register Sizes and Naming

Each 32-bit register can be accessed in smaller sizes:

```
EAX (32-bit): [         AX (16-bit)         ]
              [  AH (8-bit)  ][  AL (8-bit) ]
```

Examples:
- `EAX` = 32-bit (Extended AX)
- `AX` = lower 16 bits of EAX
- `AH` = high 8 bits of AX
- `AL` = low 8 bits of AX

This applies to EAX, EBX, ECX, and EDX. For the others:
- ESI/EDI → SI/DI (no 8-bit access)
- EBP/ESP → BP/SP (no 8-bit access)

### Special Purpose Registers

#### EIP (Instruction Pointer)

The **EIP** register is arguably the most important register in the CPU:

- **What it is**: Points to the next instruction to be executed
- **How it works**: After each instruction executes, EIP automatically increments to point to the next instruction
- **Cannot be directly modified**: You can't do `mov eip, address` (in most modes)
- **Modified by**: `jmp`, `call`, `ret`, and conditional jumps

```assembly
; Instruction at address 0x1000
mov eax, 5      ; EIP = 0x1000, after execution EIP = 0x1005
add eax, 3      ; EIP = 0x1005, after execution EIP = 0x1008
jmp 0x2000      ; EIP = 0x1008, after execution EIP = 0x2000
```

**Why EIP matters in kernel dev**: If EIP points to invalid memory or gets corrupted, the CPU will crash (triple fault in kernel mode).

#### EFLAGS Register

Stores the CPU status flags:

- **CF** (Carry Flag): Set if arithmetic operation generates a carry
- **ZF** (Zero Flag): Set if result is zero
- **SF** (Sign Flag): Set if result is negative
- **OF** (Overflow Flag): Set if signed overflow occurs
- **DF** (Direction Flag): Controls string operation direction
- **IF** (Interrupt Flag): Controls whether interrupts are enabled

```assembly
cmp eax, ebx    ; Compare eax and ebx, sets flags
je equal        ; Jump if ZF = 1 (eax == ebx)
jg greater      ; Jump if ZF = 0 and SF = OF (eax > ebx)
```

### Segment Registers

In real mode and protected mode, segment registers define memory segments:

- **CS** (Code Segment): Points to code segment
- **DS** (Data Segment): Points to data segment
- **SS** (Stack Segment): Points to stack segment
- **ES, FS, GS**: Extra segment registers

In protected mode (kernel development), these are loaded with selectors that index into the GDT (Global Descriptor Table).

---

## Memory Addressing

### Direct Addressing

```assembly
mov eax, [0x1000]    ; Load value from memory address 0x1000 into EAX
mov [0x2000], ebx    ; Store EBX value to memory address 0x2000
```

### Register Indirect

```assembly
mov eax, [ebx]       ; Load value from address stored in EBX
mov [ecx], eax       ; Store EAX to address stored in ECX
```

### Base + Displacement

```assembly
mov eax, [ebx + 4]   ; Load from address (EBX + 4)
mov [esp + 8], ecx   ; Store to address (ESP + 8)
```

### Indexed Addressing

```assembly
mov eax, [ebx + ecx]        ; Address = EBX + ECX
mov eax, [ebx + ecx*4]      ; Address = EBX + (ECX * 4)
mov eax, [ebx + ecx*4 + 8]  ; Address = EBX + (ECX * 4) + 8
```

The scale factor (1, 2, 4, or 8) is useful for array indexing:

```assembly
; Access array[i] where each element is 4 bytes
; array base in EBX, i in ECX
mov eax, [ebx + ecx*4]
```

### Size Specifiers

```assembly
mov byte [ebx], 0x42     ; Move 1 byte
mov word [ebx], 0x4242   ; Move 2 bytes (16-bit)
mov dword [ebx], 0x42424242  ; Move 4 bytes (32-bit)
```

---

## Basic Instructions

### Data Movement

#### MOV - Move Data

The most fundamental instruction:

```assembly
mov destination, source

; Examples:
mov eax, 42          ; Load immediate value 42 into EAX
mov eax, ebx         ; Copy EBX into EAX
mov eax, [0x1000]    ; Load from memory
mov [0x2000], eax    ; Store to memory
```

**Rules**:
- Cannot move memory to memory directly: `mov [addr1], [addr2]` is **invalid**
- Cannot move immediate to memory without size: use `mov dword [addr], 42`
- Both operands must be same size

#### LEA - Load Effective Address

Calculates an address without accessing memory:

```assembly
lea eax, [ebx + ecx*4 + 8]   ; EAX = EBX + ECX*4 + 8 (just the address)
```

This is different from `mov`:
```assembly
mov eax, [ebx + 8]    ; Load VALUE at (EBX + 8)
lea eax, [ebx + 8]    ; Calculate address (EBX + 8), store in EAX
```

**Common uses**:
- Efficient arithmetic: `lea eax, [eax + eax*2]` = multiply EAX by 3
- Getting addresses of variables

#### XCHG - Exchange

```assembly
xchg eax, ebx    ; Swap EAX and EBX
xchg [mem], eax  ; Swap memory and EAX (atomic on x86)
```

### Arithmetic Operations

#### ADD / SUB - Addition / Subtraction

```assembly
add eax, 5       ; EAX = EAX + 5
add eax, ebx     ; EAX = EAX + EBX
sub eax, 10      ; EAX = EAX - 10
```

Affects flags: CF, ZF, SF, OF, AF, PF

#### INC / DEC - Increment / Decrement

```assembly
inc eax          ; EAX = EAX + 1
dec ebx          ; EBX = EBX - 1
```

More efficient than `add eax, 1`. Affects all flags except CF.

#### MUL / IMUL - Multiplication

**MUL** (unsigned):
```assembly
mov eax, 5
mov ebx, 3
mul ebx          ; EAX = EAX * EBX = 15, EDX = high 32 bits (for 64-bit result)
```

**IMUL** (signed, more flexible):
```assembly
imul eax, ebx        ; EAX = EAX * EBX
imul eax, ebx, 10    ; EAX = EBX * 10
```

#### DIV / IDIV - Division

**DIV** (unsigned):
```assembly
mov edx, 0       ; Clear EDX (high 32 bits of dividend)
mov eax, 17      ; Low 32 bits of dividend
mov ebx, 5       ; Divisor
div ebx          ; EAX = 17/5 = 3 (quotient), EDX = 17%5 = 2 (remainder)
```

**Important**: Always clear EDX before unsigned division, or set it appropriately for 64-bit dividend.

### Logical Operations

#### AND / OR / XOR

```assembly
and eax, 0xFF    ; Keep only lower 8 bits of EAX
or eax, 0x100    ; Set bit 8 in EAX
xor eax, eax     ; Clear EAX (very common idiom, faster than mov eax, 0)
```

#### NOT - Bitwise NOT

```assembly
not eax          ; EAX = ~EAX (flip all bits)
```

#### TEST - Logical Compare

Like AND but only sets flags, doesn't store result:

```assembly
test eax, eax    ; Check if EAX is zero (sets ZF if zero)
jz is_zero       ; Jump if ZF is set
```

#### CMP - Compare

Like SUB but only sets flags:

```assembly
cmp eax, 42      ; Compare EAX with 42
je equal         ; Jump if equal
jl less          ; Jump if less
```

### Bit Manipulation

#### SHL / SHR - Shift Left / Right

```assembly
shl eax, 1       ; Shift left by 1 (multiply by 2)
shr eax, 2       ; Shift right by 2 (divide by 4, unsigned)
```

#### SAL / SAR - Arithmetic Shift

```assembly
sal eax, 1       ; Same as SHL
sar eax, 1       ; Arithmetic shift right (preserves sign bit)
```

#### ROL / ROR - Rotate

```assembly
rol eax, 1       ; Rotate left
ror eax, 1       ; Rotate right
```

---

## Control Flow

### Unconditional Jumps

#### JMP - Jump

```assembly
jmp label        ; Jump to label
jmp eax          ; Jump to address in EAX
jmp [eax]        ; Jump to address stored at memory address in EAX
```

### Conditional Jumps

Based on EFLAGS after a comparison:

```assembly
cmp eax, ebx
je equal         ; Jump if Equal (ZF=1)
jne not_equal    ; Jump if Not Equal (ZF=0)
jg greater       ; Jump if Greater (signed)
jl less          ; Jump if Less (signed)
ja above         ; Jump if Above (unsigned)
jb below         ; Jump if Below (unsigned)
jge greater_eq   ; Jump if Greater or Equal (signed)
jle less_eq      ; Jump if Less or Equal (signed)
jz zero          ; Jump if Zero (ZF=1, same as JE)
jnz not_zero     ; Jump if Not Zero (ZF=0, same as JNE)
```

**Signed vs Unsigned**:
- Use `jg`, `jl`, `jge`, `jle` for signed comparisons
- Use `ja`, `jb`, `jae`, `jbe` for unsigned comparisons

### Loops

#### LOOP - Decrement and Loop

```assembly
mov ecx, 10      ; Counter
loop_start:
    ; ... loop body ...
    loop loop_start  ; Decrement ECX, jump if ECX != 0
```

Equivalent to:
```assembly
    dec ecx
    jnz loop_start
```

---

## Stack Operations

The stack grows **downward** in memory (from high addresses to low addresses).

### PUSH - Push onto Stack

```assembly
push eax         ; ESP = ESP - 4, then [ESP] = EAX
push 42          ; Push immediate value
push dword [mem] ; Push from memory
```

What happens:
1. ESP decremented by operand size (4 for dword)
2. Value written to new ESP location

### POP - Pop from Stack

```assembly
pop eax          ; EAX = [ESP], then ESP = ESP + 4
pop dword [mem]  ; Pop into memory
```

What happens:
1. Value read from ESP location
2. ESP incremented by operand size

### Stack Visualization

```
High Memory
    |           |
    +-----------+
    |   Value1  | <- ESP + 8
    +-----------+
    |   Value2  | <- ESP + 4
    +-----------+
    |   Value3  | <- ESP (Top of Stack)
    +-----------+
    |           |
Low Memory
```

After `push eax`:
```
High Memory
    |           |
    +-----------+
    |   Value1  | <- ESP + 12
    +-----------+
    |   Value2  | <- ESP + 8
    +-----------+
    |   Value3  | <- ESP + 4
    +-----------+
    |    EAX    | <- ESP (New Top)
    +-----------+
Low Memory
```

### Stack Frames

A stack frame is created for each function call:

```
    +-----------+
    |   Arg 2   | <- EBP + 12
    +-----------+
    |   Arg 1   | <- EBP + 8
    +-----------+
    | Return IP | <- EBP + 4
    +-----------+
    | Saved EBP | <- EBP (Current frame base)
    +-----------+
    |  Local 1  | <- EBP - 4
    +-----------+
    |  Local 2  | <- EBP - 8, ESP
    +-----------+
```

---

## Function Calling Conventions

### cdecl (C Declaration)

The standard C calling convention:

**Caller responsibilities**:
- Push arguments right-to-left
- Clean up stack after call
- Save EAX, ECX, EDX if needed

**Callee responsibilities**:
- Save and restore EBX, ESI, EDI, EBP
- Return value in EAX

Example C function:
```c
int add(int a, int b) {
    return a + b;
}
```

Assembly implementation:
```assembly
add:
    push ebp           ; Save old base pointer
    mov ebp, esp       ; Set up new stack frame
    
    mov eax, [ebp+8]   ; Get first argument (a)
    add eax, [ebp+12]  ; Add second argument (b)
    
    mov esp, ebp       ; Restore stack pointer
    pop ebp            ; Restore base pointer
    ret                ; Return to caller
```

Calling this function:
```assembly
push 5             ; Push second argument
push 3             ; Push first argument
call add           ; Call function
add esp, 8         ; Clean up stack (2 args * 4 bytes)
; Result is now in EAX
```

### CALL - Call Function

```assembly
call function      ; Push return address, jump to function
```

What happens:
1. Push EIP+instruction_size onto stack (return address)
2. Set EIP to function address

### RET - Return from Function

```assembly
ret                ; Pop return address into EIP
ret 8              ; Pop return address, then add 8 to ESP
```

What happens:
1. Pop stack into EIP
2. Optionally adjust ESP (used in stdcall convention)

---

## Interfacing with C

### Calling Assembly from C

In your C code:
```c
extern void my_asm_function(int arg1, int arg2);

void kernel_main() {
    my_asm_function(10, 20);
}
```

In your assembly file:
```assembly
global my_asm_function

my_asm_function:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+8]    ; First argument
    mov ebx, [ebp+12]   ; Second argument
    ; ... do work ...
    
    mov esp, ebp
    pop ebp
    ret
```

### Calling C from Assembly

In your C code:
```c
void c_function(int x) {
    // ... implementation ...
}
```

In your assembly:
```assembly
extern c_function

my_code:
    push 42            ; Argument
    call c_function
    add esp, 4         ; Clean up
```

### Inline Assembly in C (GCC)

```c
int add_five(int x) {
    int result;
    asm volatile (
        "add %1, %0"
        : "=r" (result)     // Output: result in any register
        : "r" (x)           // Input: x in any register
    );
    return result;
}
```

---

## Common Patterns in Kernel Code

### 1. Enabling/Disabling Interrupts

```assembly
cli                ; Clear Interrupt Flag (disable interrupts)
; ... critical section ...
sti                ; Set Interrupt Flag (enable interrupts)
```

### 2. Loading GDT

```assembly
lgdt [gdt_descriptor]    ; Load Global Descriptor Table
```

### 3. Loading IDT

```assembly
lidt [idt_descriptor]    ; Load Interrupt Descriptor Table
```

### 4. Port I/O

```assembly
; OUT - Output to port
mov al, 0xFF
out 0x21, al       ; Write AL to port 0x21

; IN - Input from port
in al, 0x60        ; Read from port 0x60 into AL
```

### 5. Switching to Protected Mode

```assembly
mov eax, cr0       ; Read CR0 register
or eax, 1          ; Set PE (Protection Enable) bit
mov cr0, eax       ; Write back to CR0
jmp CODE_SEG:protected_mode   ; Far jump to flush pipeline
```

### 6. Setting up Stack

```assembly
mov esp, stack_top    ; Set stack pointer to top of stack area
mov ebp, esp          ; Initialize base pointer
```

### 7. Infinite Loop (Halt)

```assembly
halt:
    hlt               ; Halt CPU until interrupt
    jmp halt          ; Jump back to halt
```

### 8. String Operations

```assembly
; Move string from DS:ESI to ES:EDI
mov ecx, string_len   ; Length
cld                   ; Clear direction flag (forward)
rep movsb             ; Repeat move byte while ECX > 0
```

### 9. Zero Memory

```assembly
mov edi, buffer       ; Destination
mov ecx, buffer_size  ; Size in bytes
xor eax, eax          ; Zero (value to write)
rep stosb             ; Repeat store byte
```

### 10. Reading/Writing Control Registers

```assembly
; Read CR3 (page directory base)
mov eax, cr3

; Write CR3 (switch page directory)
mov cr3, eax

; Cannot move directly: mov cr0, cr3 is INVALID
```

---

## Quick Reference Card

### Most Common Instructions

| Instruction | Operation | Example |
|-------------|-----------|---------|
| `mov` | Move data | `mov eax, ebx` |
| `add` | Add | `add eax, 5` |
| `sub` | Subtract | `sub eax, ebx` |
| `inc` | Increment | `inc eax` |
| `dec` | Decrement | `dec ecx` |
| `push` | Push to stack | `push eax` |
| `pop` | Pop from stack | `pop eax` |
| `call` | Call function | `call func` |
| `ret` | Return | `ret` |
| `jmp` | Unconditional jump | `jmp label` |
| `je/jz` | Jump if equal/zero | `je label` |
| `jne/jnz` | Jump if not equal/zero | `jne label` |
| `cmp` | Compare | `cmp eax, 0` |
| `test` | Logical compare | `test eax, eax` |
| `and` | Bitwise AND | `and eax, 0xFF` |
| `or` | Bitwise OR | `or eax, 0x100` |
| `xor` | Bitwise XOR | `xor eax, eax` |

### Register Quick Reference

- **EAX**: Return values, accumulator
- **EBX**: General purpose (callee-saved)
- **ECX**: Counter for loops
- **EDX**: I/O operations, multiplication high bits
- **ESI**: Source for string ops
- **EDI**: Destination for string ops
- **EBP**: Stack frame base
- **ESP**: Stack pointer (current top)
- **EIP**: Instruction pointer (cannot be directly modified)

### Flags

- **ZF**: Set if result is zero
- **CF**: Set if unsigned overflow/borrow
- **SF**: Set if result is negative
- **OF**: Set if signed overflow

---

## Tips for Kernel Development

1. **Always initialize ESP**: Before calling any function or using the stack
2. **Align stack**: Keep ESP aligned to 16 bytes for modern systems
3. **Save registers**: Follow calling conventions strictly
4. **Use CLI/STI carefully**: Disabling interrupts for too long can cause issues
5. **Test incrementally**: Assembly bugs are hard to debug
6. **Comment heavily**: Assembly is hard to read later
7. **Use labels**: Make code readable with meaningful label names
8. **Watch for size mismatches**: `mov al, [ebx]` vs `mov eax, [ebx]`
9. **Remember stack direction**: Stack grows downward
10. **Handle segment registers**: In protected mode, they must be set up correctly

---

## Resources

- **Intel® 64 and IA-32 Architectures Software Developer Manuals**: The definitive reference
- **OSDev Wiki**: Excellent resource for OS development (osdev.org)
- **NASM Documentation**: If using NASM assembler
- **GDB**: For debugging assembly code

---

## Example: Complete Boot Sector

```assembly
[BITS 16]           ; 16-bit real mode
[ORG 0x7C00]        ; BIOS loads boot sector here

start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Stack grows down from boot sector
    
    ; Print message
    mov si, message
    call print_string
    
    ; Infinite loop
    jmp $

print_string:
    lodsb               ; Load byte from DS:SI into AL, increment SI
    or al, al           ; Check if zero
    jz .done
    mov ah, 0x0E        ; BIOS teletype function
    int 0x10            ; BIOS video interrupt
    jmp print_string
.done:
    ret

message db 'Hello from Assembly!', 0

times 510-($-$$) db 0   ; Pad to 510 bytes
dw 0xAA55               ; Boot signature
```

---

Good luck with your KFS-1 project! Assembly can be challenging, but it's incredibly rewarding once you understand how the CPU really works.
