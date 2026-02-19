#include <stddef.h>

/*
 * Calculates length of a string and returns it as a size_t
 */
size_t	strlen(const char *str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}


/*
 * Compares two strings and returns the difference. 0 if not
 */
int		strcmp(const char *s1, const char *s2) {
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/*
 * Fills the first n bytes of the memory that are pointed to by s with the constant byte c.
 */
void	*memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

/*
 * Copies n bytes from memory area src to dest. They must not overlap.
 * Returns a pointer to dest.
 */
void	*memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

