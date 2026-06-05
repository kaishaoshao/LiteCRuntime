/*
 * Bounded string-backed printf-family wrapper.
 *
 * This file is the real home of snprintf()/vsnprintf().
 */

#include "printf/core/printf_buffer.h"

int
snprintf(char *s, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int i = __printf_buffer_route_n(s, n, fmt, ap, __printf_core_default);
    va_end(ap);
    return i;
}

int
vsnprintf(char *s, size_t n, const char *fmt, va_list ap)
{
    return __printf_buffer_route_n(s, n, fmt, ap, __printf_core_default);
}
