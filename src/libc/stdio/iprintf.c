/*
 * Integer-profile printf wrappers.
 *
 * The FILE route is intentionally the same shape as the default printf family:
 *
 *   iprintf  -> vfiprintf(stdout, ...)
 *   fiprintf -> vfiprintf(stream, ...)
 *   vfiprintf -> __printf_file_route(..., __printf_core_integer)
 */

#include "printf/core/printf_file.h"
#include "printf/core/printf_buffer.h"

int
iprintf(const char *fmt, ...)
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    i = vfiprintf(stdout, fmt, ap);
    va_end(ap);
    return i;
}

int
viprintf(const char *fmt, va_list ap)
{
    return vfiprintf(stdout, fmt, ap);
}

int
fiprintf(FILE *stream, const char *fmt, ...)
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    i = vfiprintf(stream, fmt, ap);
    va_end(ap);
    return i;
}

int
vfiprintf(FILE *stream, const char *fmt, va_list ap)
{
    return __printf_file_route(stream, fmt, ap, __printf_core_integer);
}

int
siprintf(char *s, const char *fmt, ...)
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    i = __printf_buffer_route(s, fmt, ap, __printf_core_integer);
    va_end(ap);
    return i;
}

int
vsiprintf(char *s, const char *fmt, va_list ap)
{
    return __printf_buffer_route(s, fmt, ap, __printf_core_integer);
}
