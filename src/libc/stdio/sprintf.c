/*
 * String-backed printf-family wrapper.
 *
 * This file is the real home of sprintf()/vsprintf().
 */

#include "printf/core/printf_buffer.h"

int
sprintf(char *s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int i = __printf_buffer_route(s, fmt, ap, __printf_core_default);
    va_end(ap);
    return i;
}

int
vsprintf(char *s, const char *fmt, va_list ap)
{
    return __printf_buffer_route(s, fmt, ap, __printf_core_default);
}
