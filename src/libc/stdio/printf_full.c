/*
 * Full-profile printf wrappers.
 *
 * The FILE route intentionally has the same shape as the default and integer
 * printf families:
 *
 *   printf_full  -> vfprintf_full(stdout, ...)
 *   fprintf_full -> vfprintf_full(stream, ...)
 *   vfprintf_full -> __printf_file_route(..., __printf_core_full)
 */

#include "printf/core/printf_file.h"
#include "printf/core/printf_buffer.h"

int
printf_full(const char *fmt, ...)
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    i = vfprintf_full(stdout, fmt, ap);
    va_end(ap);
    return i;
}

int
vprintf_full(const char *fmt, va_list ap)
{
    return vfprintf_full(stdout, fmt, ap);
}

int
fprintf_full(FILE *stream, const char *fmt, ...)
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    i = vfprintf_full(stream, fmt, ap);
    va_end(ap);
    return i;
}

int
vfprintf_full(FILE *stream, const char *fmt, va_list ap)
{
    return __printf_file_route(stream, fmt, ap, __printf_core_full);
}

int
sprintf_full(char *s, const char *fmt, ...)
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    i = __printf_buffer_route(s, fmt, ap, __printf_core_full);
    va_end(ap);
    return i;
}

int
vsprintf_full(char *s, const char *fmt, va_list ap)
{
    return __printf_buffer_route(s, fmt, ap, __printf_core_full);
}

int
snprintf_full(char *s, size_t n, const char *fmt, ...)
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    i = __printf_buffer_route_n(s, n, fmt, ap, __printf_core_full);
    va_end(ap);
    return i;
}

int
vsnprintf_full(char *s, size_t n, const char *fmt, va_list ap)
{
    return __printf_buffer_route_n(s, n, fmt, ap, __printf_core_full);
}
