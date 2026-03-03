#ifndef VGA_H
#define VGA_H

#include <stddef.h>
#include <stdint.h>

/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | ((uint16_t)color << 8);
}
void outb(uint16_t port, uint8_t value);
// Direct hardware VGA buffer pointer
#define VGA_BUFFER ((uint16_t *)0xB8000)

// ── Output area (rows 0‑23) ──────────────────────────────────────────────────
void    tab_init(uint8_t tab_id);           // zero-init one tab
void    tab_output_char(char c);            // write char + scroll if needed
void    tab_output_string(const char *s);   // write string
void    tab_scroll_up(void);                // scroll output area up 1 line
void    tab_clear_output(void);             // clear rows 0‑23 of current tab

// ── Input bar (row 24) ───────────────────────────────────────────────────────
void    tab_render_input(void);             // redraw input bar from tab state
void    tab_input_putchar(char c);          // append char to input line
void    tab_input_backspace(void);          // delete last char
void    tab_input_clear(void);              // clear input buffer + redraw

// ── Tab switching ────────────────────────────────────────────────────────────
void    tab_save_and_switch(uint8_t new_tab);   // save current, load new

// ── Full screen refresh ──────────────────────────────────────────────────────
void    tab_flush_to_vga(void);             // blit current tab's screen to VGA

// ── Cursor ───────────────────────────────────────────────────────────────────
void    enable_cursor(uint8_t start, uint8_t end);
void    move_cursor(size_t x, size_t y);
void    tab_update_cursor(void);
void    tab_move_cursor_left(void);
void    tab_move_cursor_right(void);

#endif