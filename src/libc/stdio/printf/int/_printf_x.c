/*
 * Entry: %x / %X
 */

static int
__printf_int_hex_entry(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                       int *width, unsigned char conv, va_list ap, char *buf)
{
    ultoa_unsigned_t x = __printf_read_unsigned_arg(ap, *flags);
    int base = ('x' - conv) | 16;

    return __printf_int_format_base(out, stream_len, flags, prec, width, x, base, conv, buf);
}
