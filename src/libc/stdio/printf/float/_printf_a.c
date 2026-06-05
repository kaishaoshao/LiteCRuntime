/*
 * Entry: %a / %A
 */

#include "printf_float.h"

int
__printf_float_a(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                 int *width, unsigned char conv, va_list ap, struct dtoa *dtoa)
{
    unsigned char case_convert = (conv >= 'A' && conv <= 'Z') ? (unsigned char) ('a' - 'A') : 0;

    return __printf_float_format_hex(out, stream_len, flags, prec, width, case_convert, ap, dtoa);
}
