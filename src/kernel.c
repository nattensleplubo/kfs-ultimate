#include "kernel.h"
#include "printk.h"
#include "k_lib.h"

// ─── Keyboard scancode map ────────────────────────────────────────────────────
// Special values: 0 = ignored, negative = function keys
// F1‑F9 => -1 to -9
static int extended = 0;

unsigned char keyboard_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b',
    '\t',
    'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,                  /* Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0,       /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/', 0,              /* Right shift */
    '*',
    0,                  /* Alt */
    ' ',                /* Space */
    0,                  /* Caps lock */
    -1,                 /* F1 */
    -2, -3, -4, -5, -6, -7, -8, -9,    /* F2‑F9 */
    0,                  /* F10 */
    0, 0, 0, 0, 0,      /* Num lock, Scroll lock, Home, Up, PgUp */
    '-',
    0, 0, 0,            /* Left, center, Right */
    '+',
    0, 0, 0, 0, 0,      /* End, Down, PgDn, Ins, Del */
    0, 0, 0, 0, 0,      /* misc */
};

t_tab   tabs[TAB_COUNT];
uint8_t current_tab = 0;

// ─── Command dispatcher ──────────────────────────────────────────────────────

/*
 * Split a string by the first space into cmd + args.
 * cmd_buf must be at least INPUT_MAX+1 bytes.
 * Returns pointer into line for the args part (or points to '\0').
 */
static const char *split_cmd(const char *line, char *cmd_buf) {
    size_t i = 0;
    while (line[i] && line[i] != ' ' && i < INPUT_MAX) {
        cmd_buf[i] = line[i];
        i++;
    }
    cmd_buf[i] = '\0';
    if (line[i] == ' ')
        i++;
    return line + i;
}

static void cmd_help(const char *args) {
    (void)args;
    printk("commands:\n");
    printk("  help          - show this message\n");
    printk("  clear         - clear the screen\n");
    printk("  echo <text>   - print text\n");
    printk("  tab           - show current tab number\n");
    printk("  color <0-f>   - change text color (hex)\n");
}

static void cmd_clear(const char *args) {
    (void)args;
    tab_clear_output();
    tab_flush_to_vga();
}

static void cmd_echo(const char *args) {
    printk("%s\n", args);
}

static void cmd_tab(const char *args) {
    (void)args;
    printk("current tab: %d\n", current_tab + 1);
}

static void cmd_color(const char *args) {
    if (!args || !args[0]) {
        printk("usage: color <0-f>\n");
        return;
    }
    uint8_t c;
    char    ch = args[0];
    if (ch >= '0' && ch <= '9')
        c = ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        c = 10 + (ch - 'a');
    else {
        printk("invalid color. use 0-9 or a-f\n");
        return;
    }
    tabs[current_tab].color = vga_entry_color((enum vga_color)c, VGA_COLOR_BLACK);
    printk("color set\n");
}

static void run_command(const char *line) {
    char        cmd[INPUT_MAX + 1];
    const char *args;

    // Echo the command to output first
    printk("> %s\n", line);

    if (!line[0])
        return;

    args = split_cmd(line, cmd);

    if (!strcmp(cmd, "help"))
        cmd_help(args);
    else if (!strcmp(cmd, "clear"))
        cmd_clear(args);
    else if (!strcmp(cmd, "echo"))
        cmd_echo(args);
    else if (!strcmp(cmd, "tab"))
        cmd_tab(args);
    else if (!strcmp(cmd, "color"))
        cmd_color(args);
    else
        printk("unknown command: %s\n", cmd);
}

// ─── Keyboard handling ───────────────────────────────────────────────────────

static uint8_t read_keyboard(void) {
    return inb(KEYBOARD_DATA_PORT);
}

/*
 * Handle one scancode. Returns 1 if something was processed.
 */
static void handle_scancode(uint8_t scancode)
{
    // Gestion scancode étendu
    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    // Ignore releases
    if (scancode & 0x80)
        return;

    if (extended) {
        switch (scancode) {
            case 0x4B: // ←
                tab_move_cursor_left();
                break;
            case 0x4D: // →
                tab_move_cursor_right();
                break;
        }
        extended = 0;
        return;
    }

    signed char key = (signed char)keyboard_map[scancode];

    // F1-F9
    if (key <= -1 && key >= -9) {
        uint8_t new_tab = (uint8_t)(-(key + 1));
        if (new_tab != current_tab)
            tab_save_and_switch(new_tab);
        return;
    }

    if (key == '\n') {
        char cmd_copy[INPUT_MAX + 1];
        memcpy(cmd_copy, tabs[current_tab].input,
               tabs[current_tab].input_len + 1);
        tab_input_clear();
        run_command(cmd_copy);
        return;
    }

    if (key == '\b') {
        tab_input_backspace();
        return;
    }

    if (key > 0)
        tab_input_putchar((char)key);
}

// ─── Entry point ─────────────────────────────────────────────────────────────

void kernel_main(void) {
    // Initialize all tabs
    for (uint8_t i = 0; i < TAB_COUNT; i++)
        tab_init(i);

    // Display on screen
    tab_flush_to_vga();
    tab_render_input();
    enable_cursor(14, 15);
    printk("kshell v0.1  |  tabs: F1-F%d  |  type 'help'\n", TAB_COUNT);

    while (1) {
        if (keyboard_has_data()) {
            uint8_t scancode = read_keyboard();
            handle_scancode(scancode);
        }
    }
}