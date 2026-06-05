/*
 * Entry: %b
 */

static int
__printf_int_bin_entry(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                       int *width, unsigned char conv, va_list ap, char *buf)
{
    ultoa_unsigned_t x = __printf_read_unsigned_arg(ap, *flags);
    return __printf_int_format_base(out, stream_len, flags, prec, width, x, 2, conv, buf);
}
