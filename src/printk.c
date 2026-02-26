#include "printk.h"
#include "vga.h"
#include "k_lib.h"

// ─── Internal buffer writer ──────────────────────────────────────────────────

typedef struct {
    char    *buf;
    size_t   pos;
    size_t   size;  // 0 = unbounded (terminal mode)
} t_writer;

static void writer_putchar(t_writer *w, char c) {
    if (w->buf) {
        // Buffer mode: leave room for null terminator
        if (w->size == 0 || w->pos + 1 < w->size)
            w->buf[w->pos++] = c;
    } else {
        terminal_putchar(c);
    }
}

static void writer_putstr(t_writer *w, const char *s) {
    while (*s)
        writer_putchar(w, *s++);
}

// ─── Formatters ──────────────────────────────────────────────────────────────

static void fmt_int(t_writer *w, int n) {
    if (n < 0) {
        writer_putchar(w, '-');
        n = -n;
    }
    if (n >= 10)
        fmt_int(w, n / 10);
    writer_putchar(w, '0' + (n % 10));
}

static void fmt_hex(t_writer *w, unsigned int n, int prefix) {
    const char  *hex_chars = "0123456789abcdef";
    char         buf[16];
    int          i = 0;

    if (prefix)
        writer_putstr(w, "0x");
    if (n == 0) {
        writer_putchar(w, '0');
        return;
    }
    while (n > 0) {
        buf[i++] = hex_chars[n % 16];
        n /= 16;
    }
    while (i > 0)
        writer_putchar(w, buf[--i]);
}

// ─── Core formatter ──────────────────────────────────────────────────────────

static void vformat(t_writer *w, const char *format, va_list args) {
    while (*format != '\0') {
        if (*format == '%') {
            format++;
            if (*format == 'c') {
                writer_putchar(w, (char)va_arg(args, int));
            } else if (*format == 's') {
                writer_putstr(w, va_arg(args, const char *));
            } else if (*format == 'd') {
                fmt_int(w, va_arg(args, int));
            } else if (*format == 'x') {
                fmt_hex(w, va_arg(args, unsigned int), 0);
            } else if (*format == 'p') {
                fmt_hex(w, (unsigned int)(uintptr_t)va_arg(args, void *), 1);
            } else if (*format == '%') {
                writer_putchar(w, '%');
            }
        } else {
            writer_putchar(w, *format);
        }
        format++;
    }
}

// ─── Public API ──────────────────────────────────────────────────────────────

/*
 * Prints a formatted string to the terminal (like printf).
 */
void printk(const char *format, ...) {
    t_writer w = { .buf = NULL, .pos = 0, .size = 0 };
    va_list args;

    va_start(args, format);
    vformat(&w, format, args);
    va_end(args);
}

/*
 * Formats a string into buf (at most buf_size - 1 chars + null terminator).
 * Returns the number of characters written (excluding the null terminator).
 * Safe: always null-terminates if buf_size > 0.
 */
int sprintk(char *buf, size_t buf_size, const char *format, ...) {
    t_writer w = { .buf = buf, .pos = 0, .size = buf_size };
    va_list args;

    if (!buf || buf_size == 0)
        return 0;

    va_start(args, format);
    vformat(&w, format, args);
    va_end(args);

    buf[w.pos] = '\0';
    return (int)w.pos;
}