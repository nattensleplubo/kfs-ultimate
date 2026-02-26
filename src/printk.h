#ifndef PRINTK_H
# define PRINTK_H

#include <stdarg.h>
#include <stddef.h>

void	printk(const char *format, ...);
int		sprintk(char *buf, size_t buf_size, const char *format, ...);

#endif