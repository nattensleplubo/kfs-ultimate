#include "printk.h"
#include "k_lib.h"

void    printk(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            if (*format == 'c') {
                
            }
        }
    }
}