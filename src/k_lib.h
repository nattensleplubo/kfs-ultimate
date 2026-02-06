#ifndef K_LIB_H
#define K_LIB_H

#include <stddef.h>

size_t	strlen(const char *str);
int		strcmp(const char *s1, const char *s2);
void	*memset(void *s, int c, size_t n) ;
void	*memcpy(void *dest, const void *src, size_t n);


#endif
