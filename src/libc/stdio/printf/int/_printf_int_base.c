/*
 * Shared helper for unsigned/octal/hex/base-N formatting.
 */

static int
__printf_int_format_base(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                         int *width, ultoa_unsigned_t x, int base, unsigned char prefix_c, char *buf)
{
    int local_flags = *flags;
    int local_prec = *prec;
    int local_width = *width;
    int buf_len;

    local_flags &= ~(PRINTF_FLAG_PLUS | PRINTF_FLAG_SPACE);

    if (x == 0)
        local_flags &= ~PRINTF_FLAG_ALT_FORM;

#if !PRINTF_CAP_SHRINK
    if (x == 0 && (local_flags & PRINTF_FLAG_PRECISION) && local_prec == 0)
        buf_len = 0;
    else
#endif
        buf_len = __ultoa_invert(x, buf, base) - buf;

#if !PRINTF_CAP_SHRINK
    {
        int len = buf_len;

        if (local_flags & PRINTF_FLAG_PRECISION) {
            local_flags &= ~PRINTF_FLAG_ZERO_FILL;

            if (len < local_prec) {
                len = local_prec;

                if (prefix_c == '\0')
                    local_flags &= ~PRINTF_FLAG_ALT_FORM;
            }
        }

        if (local_flags & PRINTF_FLAG_ALT_FORM) {
            len += 1;
            if (prefix_c != '\0')
                len += 1;
        }

        if (!(local_flags & PRINTF_FLAG_LEFT_ADJ)) {
            if (local_flags & PRINTF_FLAG_ZERO_FILL) {
                local_prec = buf_len;
                if (len < local_width) {
                    local_prec += local_width - len;
                    len = local_width;
                }
            }
            if (local_width > len
                && __printf_emit_repeat(out, stream_len, ' ', (size_t) (local_width - len)) < 0)
                return -1;
        }

        local_width -= len;

        if (local_flags & PRINTF_FLAG_ALT_FORM) {
            if (__printf_emit(out, stream_len, '0') < 0)
                return -1;
            if (prefix_c != '\0' && __printf_emit(out, stream_len, prefix_c) < 0)
                return -1;
        }

        if (local_prec > buf_len) {
            if (__printf_emit_repeat(out, stream_len, '0', (size_t) (local_prec - buf_len)) < 0)
                return -1;
            local_prec = buf_len;
        }
    }
#else
    if (local_flags & PRINTF_FLAG_ALT_FORM) {
        if (__printf_emit(out, stream_len, '0') < 0)
            return -1;
        if (prefix_c != '\0' && __printf_emit(out, stream_len, prefix_c) < 0)
            return -1;
    }
#endif

    if (__printf_emit_reversed_span(out, stream_len, buf, (size_t) buf_len) < 0)
        return -1;

    *flags = (uint16_t) local_flags;
    *prec = local_prec;
    *width = local_width;
    return 0;
}

/*
 * Shared helper for pointer formatting.
 */

static int
__printf_int_format_pointer(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                            int *width, ultoa_unsigned_t x, char *buf)
{
    uint16_t local_flags = *flags | PRINTF_FLAG_ALT_FORM;

    if (sizeof(void *) > sizeof(int))
        local_flags |= PRINTF_FLAG_LONG;

    return __printf_int_format_base(out, stream_len, &local_flags, prec, width, x, 16, 'x', buf);
}

/*
 * Entry: %p
 */

static int
__printf_int_pointer_entry(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                           int *width, va_list ap, char *buf)
{
    ultoa_unsigned_t x = __printf_read_unsigned_arg(ap, *flags);
    return __printf_int_format_pointer(out, stream_len, flags, prec, width, x, buf);
}
