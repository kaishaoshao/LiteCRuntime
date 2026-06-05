/*
 * Shared helper for signed decimal formatting.
 */

static int
__printf_int_format_dec(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                        int *width, ultoa_signed_t x_s, char *buf)
{
    int local_flags = *flags;
    int local_prec = *prec;
    int local_width = *width;
    int buf_len;

    if (x_s < 0) {
        x_s = (ultoa_signed_t) - (ultoa_unsigned_t) x_s;
        local_flags |= PRINTF_FLAG_NEGATIVE;
    }

    local_flags &= ~PRINTF_FLAG_ALT_FORM;

#if !PRINTF_CAP_SHRINK
    if (x_s == 0 && (local_flags & PRINTF_FLAG_PRECISION) && local_prec == 0)
        buf_len = 0;
    else
#endif
        buf_len = __ultoa_invert(x_s, buf, 10) - buf;

#if !PRINTF_CAP_SHRINK
    {
        int len = buf_len;

        if (local_flags & PRINTF_FLAG_PRECISION) {
            local_flags &= ~PRINTF_FLAG_ZERO_FILL;
            if (len < local_prec)
                len = local_prec;
        }

        if (local_flags & (PRINTF_FLAG_NEGATIVE | PRINTF_FLAG_PLUS | PRINTF_FLAG_SPACE))
            len += 1;

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

        if (local_flags & (PRINTF_FLAG_NEGATIVE | PRINTF_FLAG_PLUS | PRINTF_FLAG_SPACE)) {
            unsigned char z = ' ';
            if (local_flags & PRINTF_FLAG_PLUS)
                z = '+';
            if (local_flags & PRINTF_FLAG_NEGATIVE)
                z = '-';
            if (__printf_emit(out, stream_len, z) < 0)
                return -1;
        }

        if (local_prec > buf_len) {
            if (__printf_emit_repeat(out, stream_len, '0', (size_t) (local_prec - buf_len)) < 0)
                return -1;
            local_prec = buf_len;
        }
    }
#else
    if (local_flags & PRINTF_FLAG_NEGATIVE) {
        if (__printf_emit(out, stream_len, '-') < 0)
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
