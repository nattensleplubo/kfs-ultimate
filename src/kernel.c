#include	<stdint.h>
#include	"k_lib.h"

// VGA DEFINES
# define	VGA_MEMORY	0xB8000
# define	VGA_WIDTH	80
# define	VGA_HEIGHT  25

# define	TAB_COUNT		9
# define	SCREEN_CELLS	(VGA_WIDTH * VGA_HEIGHT)

# define	KEYBOARD_DATA_PORT		0x60
# define	KEYBOARD_STATUS_PORT	0x64

// CONSTANTS
size_t		terminal_row;
size_t		terminal_column;
uint8_t		terminal_color;
uint16_t*	terminal_buffer = (uint16_t*)VGA_MEMORY;

// VGA HELPERS
/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

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

typedef struct	s_tab {
	uint16_t	buffer[SCREEN_CELLS];
	size_t		row;
	size_t		col;
	uint8_t		color;
} t_tab;

static t_tab	tabs[TAB_COUNT];
static int		current_tab = 0;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}


//FOR KEYBOARD ENTRY
static inline uint8_t	inb(uint16_t port) {
	uint8_t	result;
	__asm__ volatile("in %1, %0" : "=a"(result) : "Nd"(port));
	return result;
}

static inline int	keyboard_has_data(void) {
	return inb(KEYBOARD_STATUS_PORT) & 0x01;
}

uint8_t	read_keyboard(void) {
	return inb(KEYBOARD_DATA_PORT);
}

void	handle_keyboard(void) {
	if (keyboard_has_data()) {
		uint8_t	scancode = read_keyboard();
		if (!(scancode & 0x80)) {
			terminal_putchar(keyboard_map[scancode]);
		}
	}
}


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

void	terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t i = y * VGA_WIDTH + x;
	terminal_buffer[i] = vga_entry(c, color);
}

void	terminal_putchar(char c) 
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void	terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void	terminal_writestring(const char* data) 
{
	terminal_write(data, k_strlen(data));
}

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

void	terminal_writestring_at(const char *str, size_t x, size_t y) {
	terminal_setpos(x, y);
	terminal_writestring(str);
}

static void delay(void)
{
	for (volatile uint32_t i = 0; i < 30000000; i++)
		__asm__ volatile("nop");
}

////////// MAIN //////////
void	kernel_main(void) {
	terminal_init();
	terminal_writestring("Hello, kernel World!\n");
	while (1) {
		handle_keyboard();
	}
}
