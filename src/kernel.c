#include	<stdint.h>

# define	VGA_MEMORY	0xB8000
# define	VGA_WIDTH	80

void	terminal_putchar(char c, uint8_t color, int x, int y) {
	uint16_t *vga = (uint16_t*)VGA_MEMORY;
	vga[y * VGA_WIDTH + x] = (uint16_t)c | (uint16_t)color << 8;
}

void	terminal_writestring(const char *str) {
	uint16_t *vga = (uint16_t*)VGA_MEMORY;
	int i = 0;
	while (str[i]) {
		vga[i] = (uint16_t)str[i] | 0x0F00; // White on black
		i++;
	}
}

void	kernel_main(void) {
	terminal_writestring("Hello from my kernel!");
	while(1);
}
