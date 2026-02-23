#include "vga.h"
#include "kernel.h"

// INIT
void	terminal_init(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void	terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void	buffer_setcolor(uint8_t color, uint8_t buffer) {
	tabs[buffer].color = color;
}

void	terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t i = y * VGA_WIDTH + x;
	terminal_buffer[i] = vga_entry(c, color);
}

void	buffer_putentryat(char c, uint8_t buffer, size_t x, size_t y) {
	const size_t i = y * VGA_WIDTH + x;
	terminal_buffer[i] = vga_entry(c, tabs[buffer].color);
}

void	terminal_putchar(char c) {
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void	buffer_putchar(char c, uint8_t buffer) {
	buffer_putentryat(c, buffer, tabs[buffer].col, tabs[buffer].row);
	if (++tabs[buffer].col == VGA_WIDTH) {
		tabs[buffer].col = 0;
		if (tabs[buffer].row == VGA_HEIGHT)
			tabs[buffer].row = 0;
	}
}

void	terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void	buffer_write(const char* data, size_t size, uint8_t buffer) {
	for (size_t i = 0; i < size; i++)
		buffer_putchar(data[i], buffer);
}

void	terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}

void	buffer_writestring(const char* data, uint8_t buffer) {
	buffer_write(data, strlen(data), buffer);
}

//	Clears the terminal
void	terminal_clear(void) {
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
	terminal_row = 0;
	terminal_column = 0;
}

void	terminal_setpos(size_t x, size_t y) {
	terminal_column = x;
	terminal_row = y;
}

void	buffer_setpos(size_t x, size_t y, uint8_t buffer) {
	tabs[buffer].col = x;
	tabs[buffer].row = y;
}

void	terminal_writestring_at(const char *str, size_t x, size_t y) {
	terminal_setpos(x, y);
	terminal_writestring(str);
}

void	buffer_writestring_at(const char* str, size_t x, size_t y, uint8_t buffer) {
	buffer_setpos(x, y, buffer);
	buffer_writestring(str, buffer);
}

void	terminal_replace_vga_memory(uint8_t buffer) {
	int i = 0;
	while (i < SCREEN_CELLS) {
		terminal_buffer[i] = tabs[buffer].buffer[i];
		i++;
	}
}