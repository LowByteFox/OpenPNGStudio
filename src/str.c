#include <stdio.h>
#include <str.h>

size_t sized_strncpy(char *dest, const char *src, size_t n)
{
    size_t count = 0;

    for (; n > 0 && (*dest = *src); dest++, src++, count++, n--);

    return count;
}

char* strchrnul(const char *s, int c)
{
    while (*s) {
        if ((char) c == *s)
            break;
        s++;
    }

    return (char*) s;
}
