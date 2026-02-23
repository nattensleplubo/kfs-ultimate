#include "printk.h"
#include "vga.h"
#include "k_lib.h"

static void print_int(int n) {
    if (n < 0) {
        terminal_putchar('-');
        n = -n;
    }
    if (n >= 10)
        print_int(n / 10);
    terminal_putchar('0' + (n % 10));
}

static void print_hex(unsigned int n, int is_pointer) {
    const char *hex_chars = "0123456789abcdef";
    char        buf[16];
    int         i = 0;

    if (is_pointer) {
        terminal_writestring("0x");
    }
    if (n == 0) {
        terminal_putchar('0');
        return;
    }
    while (n > 0) {
        buf[i++] = hex_chars[n % 16];
        n /= 16;
    }
    while (i > 0)
        terminal_putchar(buf[--i]);
}

void printk(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            if (*format == 'c') {
                char c = (char)va_arg(args, int);
                terminal_putchar(c);
            } else if (*format == 's') {
                const char *str = va_arg(args, const char *);
                terminal_writestring(str);
            } else if (*format == 'd') {
                int n = va_arg(args, int);
                print_int(n);
            } else if (*format == 'x') {
                unsigned int n = va_arg(args, unsigned int);
                print_hex(n, 0);
            } else if (*format == 'p') {
                void *ptr = va_arg(args, void *);
                print_hex((unsigned int)(uintptr_t)ptr, 1);
            } else if (*format == '%') {
                terminal_putchar('%');
            }
        } else {
            terminal_putchar(*format);
        }
        format++;
    }

    va_end(args);
}