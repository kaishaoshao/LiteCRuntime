/*
 * Entry: %o
 */

static int
__printf_int_oct_entry(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                       int *width, va_list ap, char *buf)
{
    ultoa_unsigned_t x = __printf_read_unsigned_arg(ap, *flags);
    return __printf_int_format_base(out, stream_len, flags, prec, width, x, 8, '\0', buf);
}
