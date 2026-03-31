#include "vga.h"
#include "kernel.h"
#include "k_lib.h"

// ── Cursor ──────────────────────────────────────────────────────────────────

// Enabling the Cursor
// allows you to set the start and end scanlines, the rows where the cursor starts and ends. 
// The highest scanline is 0 and the lowest scanline is the maximum scanline (usually 15).
void enable_cursor(uint8_t start, uint8_t end)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | (start & 0x1F));

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | (end & 0x1F));
}

// Moving the Cursor
// Update the cursor's location every time a new character is displayed. 
// It would be faster to instead only update it after printing an entire string.

void move_cursor(size_t x, size_t y)
{
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void tab_move_cursor_left(void)
{
    t_tab *t = &tabs[current_tab];

    if (t->cursor_pos > 0)
        t->cursor_pos--;
    
    tab_update_cursor();
}

void tab_move_cursor_right(void)
{
    t_tab *t = &tabs[current_tab];
    
    if (t->cursor_pos < t->input_len)
    t->cursor_pos++;

    tab_update_cursor();
}

void tab_update_cursor(void)
{
    t_tab *t = &tabs[current_tab];
    move_cursor(2 + t->cursor_pos, SHELL_INPUT_ROW);
}


void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// ── Helpers ──────────────────────────────────────────────────────────────────

// Write a cell directly into the current tab's screen buffer
static void tab_set_cell(size_t x, size_t y, char c, uint8_t color) {
    tabs[current_tab].screen[y * VGA_WIDTH + x] = vga_entry(c, color);
}

// Read a cell from the current tab's screen buffer
// /!\ UNUSED FOR NOW
// static uint16_t tab_get_cell(size_t x, size_t y) {
//     return tabs[current_tab].screen[y * VGA_WIDTH + x];
// }

// ── Tab init ─────────────────────────────────────────────────────────────────

void tab_init(uint8_t tab_id) {
    t_tab   *t = &tabs[tab_id];	
	uint8_t  color = vga_entry_color(tab_id + 1, VGA_COLOR_BLACK);
	if (tab_id == 0)
		color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    t->out_row   = 0;
    t->out_col   = 0;
    t->color     = color;
    t->input_len = 0;
    t->input[0]  = '\0';
    t->cursor_pos = 0;

    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            t->screen[y * VGA_WIDTH + x] = vga_entry(' ', color);

    // Draw the input bar with a prompt
    uint8_t bar_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    for (size_t x = 0; x < VGA_WIDTH; x++)
        t->screen[SHELL_INPUT_ROW * VGA_WIDTH + x] = vga_entry(' ', bar_color);
    t->screen[SHELL_INPUT_ROW * VGA_WIDTH + 0] = vga_entry('>', bar_color);
    t->screen[SHELL_INPUT_ROW * VGA_WIDTH + 1] = vga_entry(' ', bar_color);
}

// ── Flush ────────────────────────────────────────────────────────────────────

void tab_flush_to_vga(void) {
    uint16_t *vga = VGA_BUFFER;
    for (size_t i = 0; i < SCREEN_CELLS; i++)
        vga[i] = tabs[current_tab].screen[i];
}

// ── Scrolling ────────────────────────────────────────────────────────────────

void tab_scroll_up(void) {
    t_tab *t = &tabs[current_tab];

    // Shift rows 1‑(SHELL_OUTPUT_ROWS-1) up by one
    for (size_t y = 0; y < SHELL_OUTPUT_ROWS - 1; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            t->screen[y * VGA_WIDTH + x] = t->screen[(y + 1) * VGA_WIDTH + x];

    // Clear the last output row
    for (size_t x = 0; x < VGA_WIDTH; x++)
        t->screen[(SHELL_OUTPUT_ROWS - 1) * VGA_WIDTH + x] = vga_entry(' ', t->color);

    // Keep out_row pinned at the last output row, out_col unchanged
    t->out_row = SHELL_OUTPUT_ROWS - 1;
}

// ── Output ───────────────────────────────────────────────────────────────────

/*
 * Write a single character to the output area of the current tab.
 * Handles '\n' and auto-scrolls when we reach row SHELL_OUTPUT_ROWS.
 */
void tab_output_char(char c) {
    t_tab *t = &tabs[current_tab];

    if (c == '\n') {
        t->out_col = 0;
        t->out_row++;
        if (t->out_row >= SHELL_OUTPUT_ROWS)
            tab_scroll_up();
        return;
    }

    tab_set_cell(t->out_col, t->out_row, c, t->color);
    t->out_col++;

    if (t->out_col >= VGA_WIDTH) {
        t->out_col = 0;
        t->out_row++;
        if (t->out_row >= SHELL_OUTPUT_ROWS)
            tab_scroll_up();
    }
}

void tab_output_string(const char *s) {
    while (*s)
        tab_output_char(*s++);
}

void tab_clear_output(void) {
    t_tab *t = &tabs[current_tab];

    for (size_t y = 0; y < SHELL_OUTPUT_ROWS; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            t->screen[y * VGA_WIDTH + x] = vga_entry(' ', t->color);
    t->out_row = 0;
    t->out_col = 0;
}

// ── Input bar ────────────────────────────────────────────────────────────────

/*
 * Redraw the entire input bar from the current tab's input buffer.
 * Layout: "> " + input chars + spaces to fill width
 */
void tab_render_input(void) {
    t_tab  *t         = &tabs[current_tab];
    uint8_t bar_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    uint16_t *vga     = VGA_BUFFER;

    // Prompt
    t->screen[SHELL_INPUT_ROW * VGA_WIDTH + 0] = vga_entry('>', bar_color);
    t->screen[SHELL_INPUT_ROW * VGA_WIDTH + 1] = vga_entry(' ', bar_color);

    // Input text (positions 2 … VGA_WIDTH-1)
    for (size_t x = 2; x < VGA_WIDTH; x++) {
        char ch = (x - 2 < t->input_len) ? t->input[x - 2] : ' ';
        t->screen[SHELL_INPUT_ROW * VGA_WIDTH + x] = vga_entry(ch, bar_color);
    }

    // Blit just the input row directly to VGA (fast path)
    for (size_t x = 0; x < VGA_WIDTH; x++)
        vga[SHELL_INPUT_ROW * VGA_WIDTH + x] = t->screen[SHELL_INPUT_ROW * VGA_WIDTH + x];
    tab_update_cursor();
}

void tab_input_putchar(char c)
{
    t_tab *t = &tabs[current_tab];

    if (t->input_len >= INPUT_MAX)
        return;

    // Décaler à droite
    for (size_t i = t->input_len; i > t->cursor_pos; i--)
        t->input[i] = t->input[i - 1];

    t->input[t->cursor_pos] = c;

    t->input_len++;
    t->cursor_pos++;
    t->input[t->input_len] = '\0';

    tab_render_input();
}

void tab_input_backspace(void)
{
    t_tab *t = &tabs[current_tab];

    if (t->cursor_pos == 0)
        return;

    for (size_t i = t->cursor_pos - 1; i < t->input_len - 1; i++)
        t->input[i] = t->input[i + 1];

    t->input_len--;
    t->cursor_pos--;
    t->input[t->input_len] = '\0';

    tab_render_input();
}

void tab_input_clear(void) {
    t_tab *t = &tabs[current_tab];
    t->input_len = 0;
    t->input[0]  = '\0';
    t->cursor_pos = 0;
    tab_render_input();
}

// ── Tab switching ────────────────────────────────────────────────────────────

void tab_save_and_switch(uint8_t new_tab) {
    // Current tab is already up-to-date in tabs[current_tab].screen
    current_tab = new_tab;
    tab_flush_to_vga();
    tab_render_input();
    tab_update_cursor(); 
}