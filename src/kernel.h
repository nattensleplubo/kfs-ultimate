#ifndef KERNEL_H

#define KERNEL_H

// LIBRARIES INCLUDES
#include	<stdint.h>
#include	"k_lib.h"
#include    "vga.h"

// VGA DEFINES
# define	VGA_MEMORY	0xB8000
# define	VGA_WIDTH	80
# define	VGA_HEIGHT  25

# define	TAB_COUNT		9
# define	SCREEN_CELLS	(VGA_WIDTH * VGA_HEIGHT)

# define	KEYBOARD_DATA_PORT		0x60
# define	KEYBOARD_STATUS_PORT	0x64

// STRUCTURES
typedef struct	s_tab {
	uint16_t	buffer[SCREEN_CELLS];
	size_t		row;
	size_t		col;
	uint8_t		color;
} t_tab;

// GLOBALS
static size_t		terminal_row;
static size_t		terminal_column;
static uint8_t		terminal_color;
static uint16_t*	terminal_buffer = (uint16_t*)VGA_MEMORY;
static t_tab		tabs[TAB_COUNT];
static uint8_t		current_tab = 0;

// KEYBOARD MAP
extern unsigned char keyboard_map[128];

//########################## START KEYB FUNCS ##########################//
static inline uint8_t	inb(uint16_t port) {
	uint8_t	result;
	__asm__ volatile("in %1, %0" : "=a"(result) : "Nd"(port));
	return result;
}

static inline int	keyboard_has_data(void) {
	return inb(KEYBOARD_STATUS_PORT) & 0x01;
}

#endif