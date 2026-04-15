ENTRY(_start)

SECTIONS {
    .gdt 0x00000800 : {
        *(.gdt)
    }

	. = 1M;

	.text : {
		*(.multiboot)
		*(.text)
	}

	.rodata : {
		*(.rodata)
	}

	.data : {
		*(.data)
	}

	.bss : {
		*(.bss)
	}
}
