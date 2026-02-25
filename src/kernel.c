#include "kernel.h"
#include "printk.h"

unsigned char keyboard_map[128] = {
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
	'9', '0', '-', '=', '\b',	/* Backspace */
	'\t',			/* Tab */
	'q', 'w', 'e', 'r',	/* 19 */
	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
	0,			/* 29   - Control */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
	'\'', '`',   0,		/* Left shift */
	'\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
	'm', ',', '.', '/',   0,				/* Right shift */
	'*',
	0,	/* Alt */
	' ',	/* Space bar */
	0,	/* Caps lock */
	-1,	/* 59 - F1 key ... > */
	-2,   -3,   -4,   -5,   -6,   -7,   -8,   -9,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
	'-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
	'+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

size_t		terminal_row;
size_t		terminal_column;
uint8_t		terminal_color;
uint16_t*	terminal_buffer = (uint16_t*)VGA_MEMORY;
t_tab		tabs[TAB_COUNT];
uint8_t		current_tab = 0;

t_tab	*get_tab() {
	return tabs;
}

uint8_t	read_keyboard(void) {
	return inb(KEYBOARD_DATA_PORT);
}

void	handle_keyboard(void) {
	if (keyboard_has_data()) {
		uint8_t	scancode = read_keyboard();
		if (!(scancode & 0x80)) {
			if (keyboard_map[scancode] != '\n')
				terminal_putchar(keyboard_map[scancode]);
			else {
				printk("c%d r%d", terminal_column, terminal_row);
			}
				
		}
	}
}


static void delay(void)
{
	for (volatile uint32_t i = 0; i < 30000000; i++)
		__asm__ volatile("nop");
}

////////// MAIN //////////
void	kernel_main(void) {
	terminal_init();
	printk("my favorite letter is %c and my name is %s and i am %x years old\n", 's', "nathan", 42);
	while (1) {
		handle_keyboard();
	}
}
