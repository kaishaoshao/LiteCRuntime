#include "printf/core/printf_file.h"

int
vfprintf(FILE *stream, const char *fmt, va_list ap)
{
    return __printf_file_route(stream, fmt, ap, __printf_core_default);
}
