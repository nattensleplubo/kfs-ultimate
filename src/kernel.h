#ifndef KERNEL_H
#define KERNEL_H

// LIBRARIES INCLUDES
#include <stdint.h>
#include "k_lib.h"
#include "vga.h"

// VGA DEFINES
# define VGA_MEMORY     0xB8000
# define VGA_WIDTH      80
# define VGA_HEIGHT     25

# define TAB_COUNT      9
# define SCREEN_CELLS   (VGA_WIDTH * VGA_HEIGHT)

# define KEYBOARD_DATA_PORT     0x60
# define KEYBOARD_STATUS_PORT   0x64

// Shell layout
# define SHELL_OUTPUT_ROWS  (VGA_HEIGHT - 1)   // rows 0‑23 are output
# define SHELL_INPUT_ROW    (VGA_HEIGHT - 1)    // row 24 is the input bar
# define INPUT_MAX          78                  // max chars in input line (80 - 2)

// STRUCTURES
typedef struct s_tab {
    uint16_t    screen[SCREEN_CELLS];   // saved VGA cells for this tab
    size_t      out_row;                // next output row (0‑23)
    size_t      out_col;                // next output column
    uint8_t     color;                  // current fg/bg color

    char        input[INPUT_MAX + 1];   // current input line
    size_t      input_len;              // chars typed so far
    size_t      cursor_pos; 
} t_tab;

// GLOBALS
extern t_tab    tabs[TAB_COUNT];
extern uint8_t  current_tab;

// KEYBOARD MAP
extern unsigned char keyboard_map[128];

// INLINE KEYBOARD IO
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("in %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}



static inline int keyboard_has_data(void) {
    return inb(KEYBOARD_STATUS_PORT) & 0x01;
}

// FUNCTION PROTOTYPES
void    kernel_main(void);

#endif