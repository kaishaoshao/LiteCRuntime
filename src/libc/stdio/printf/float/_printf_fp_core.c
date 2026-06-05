/*
 * Shared float backend helpers for decimal and hexadecimal formatting.
 */

#include "printf_float.h"
#include "../core/printf_out_core.h"
#include "../dtoa.h"

static int
__printf_emit_fp_char(struct __printf_out *out, int *stream_len, unsigned char ch)
{
    return __printf_out_write(out, stream_len, (const char *) &ch, 1);
}

static int
__printf_emit_fp_repeat(struct __printf_out *out, int *stream_len, unsigned ch, size_t count)
{
    char chunk[16];
    size_t i;

    for (i = 0; i < sizeof(chunk); ++i)
        chunk[i] = (char) ch;

    while (count != 0) {
        size_t emit_len = count < sizeof(chunk) ? count : sizeof(chunk);

        if (__printf_out_write(out, stream_len, chunk, emit_len) < 0)
            return -1;
        count -= emit_len;
    }
    return 0;
}

/*
 * Shared helper for %f / %e / %g decimal float formatting.
 */

int
__printf_float_format_dec(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                          int *width, unsigned char conv, unsigned char case_convert, va_list ap,
                          struct dtoa *dtoa)
{
#define TOCASE_LOCAL(ch) ((ch) - case_convert)
    uint16_t local_flags = *flags;
    int local_prec = *prec;
    int local_width = *width;
    uint8_t sign;
    uint8_t ndigs;
    int exp;
    int n;
    uint8_t ndigs_exp;

#if PRINTF_CAP_LONG_DOUBLE
    if ((local_flags & (PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE))
        == (PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE)) {
        PRINTF_LONG_DOUBLE_TYPE fval;
        int ndecimal = 0;
        bool fmode = false;

        fval = PRINTF_LONG_DOUBLE_ARG(ap);

        if (!(local_flags & PRINTF_FLAG_PRECISION))
            local_prec = 6;
        if (conv == 'e') {
            ndigs = local_prec + 1;
            local_flags |= PRINTF_FLAG_FLOAT_EXP;
        } else if (conv == 'f') {
            ndigs = LONG_FLOAT_MAX_DIG;
            ndecimal = local_prec;
            local_flags |= PRINTF_FLAG_FLOAT_FIX;
            fmode = true;
        } else {
            conv += 'e' - 'g';
            ndigs = local_prec;
            if (ndigs < 1)
                ndigs = 1;
        }

        if (ndigs > LONG_FLOAT_MAX_DIG)
            ndigs = LONG_FLOAT_MAX_DIG;

        ndigs = __lfloat_d_engine(fval, dtoa, ndigs, fmode, ndecimal);
        exp = dtoa->exp;
        ndigs_exp = 2;
    } else
#endif
    {
        FLOAT_UINT fval;
        int ndecimal = 0;
        bool fmode = false;

        fval = PRINTF_FLOAT_ARG(ap);

        if (!(local_flags & PRINTF_FLAG_PRECISION))
            local_prec = 6;
        if (conv == 'e') {
            ndigs = local_prec + 1;
            local_flags |= PRINTF_FLAG_FLOAT_EXP;
        } else if (conv == 'f') {
            ndigs = FLOAT_MAX_DIG;
            ndecimal = local_prec;
            local_flags |= PRINTF_FLAG_FLOAT_FIX;
            fmode = true;
        } else {
            conv += 'e' - 'g';
            ndigs = local_prec;
            if (ndigs < 1)
                ndigs = 1;
        }

        if (ndigs > FLOAT_MAX_DIG)
            ndigs = FLOAT_MAX_DIG;

        ndigs = __float_d_engine(fval, dtoa, ndigs, fmode, ndecimal);
        exp = dtoa->exp;
        ndigs_exp = 2;
    }

    if (exp < -9 || 9 < exp)
        ndigs_exp = 2;
    if (exp < -99 || 99 < exp)
        ndigs_exp = 3;
#if PRINTF_FLOAT_CAP_64
    if (exp < -999 || 999 < exp)
        ndigs_exp = 4;
#if PRINTF_FLOAT_CAP_LARGE
    if (exp < -9999 || 9999 < exp)
        ndigs_exp = 5;
#endif
#endif

    sign = 0;
    if (dtoa->flags & DTOA_MINUS)
        sign = '-';
    else if (local_flags & PRINTF_FLAG_PLUS)
        sign = '+';
    else if (local_flags & PRINTF_FLAG_SPACE)
        sign = ' ';

    if (dtoa->flags & (DTOA_NAN | DTOA_INF)) {
        char word[4];

        ndigs = sign ? 4 : 3;
        if (local_width > ndigs) {
            local_width -= ndigs;
            if (!(local_flags & PRINTF_FLAG_LEFT_ADJ)) {
                if (__printf_emit_fp_repeat(out, stream_len, ' ', (size_t) local_width) < 0)
                    return -1;
            }
        } else {
            local_width = 0;
        }
        if (sign && __printf_emit_fp_char(out, stream_len, sign) < 0)
            return -1;
        word[0] = (dtoa->flags & DTOA_NAN) ? 'n' : 'i';
        word[1] = (dtoa->flags & DTOA_NAN) ? 'a' : 'n';
        word[2] = (dtoa->flags & DTOA_NAN) ? 'n' : 'f';
        word[0] = (char) TOCASE_LOCAL(word[0]);
        word[1] = (char) TOCASE_LOCAL(word[1]);
        word[2] = (char) TOCASE_LOCAL(word[2]);
        if (__printf_out_write(out, stream_len, word, 3) < 0)
            return -1;
    } else {
        if (!(local_flags & (PRINTF_FLAG_FLOAT_EXP | PRINTF_FLAG_FLOAT_FIX))) {
            int req_prec;

            if (local_prec == 0)
                local_prec = 1;

            while (ndigs > 0 && dtoa->digits[ndigs - 1] == '0')
                ndigs--;

            req_prec = local_prec;

            if (!(local_flags & PRINTF_FLAG_ALT_FORM))
                local_prec = ndigs;

            if (-4 <= exp && exp < req_prec) {
                local_flags |= PRINTF_FLAG_FLOAT_FIX;

                if (exp < local_prec)
                    local_prec = local_prec - (exp + 1);
                else
                    local_prec = 0;
            } else {
                local_prec = local_prec - 1;
            }
        }

        if (local_flags & PRINTF_FLAG_FLOAT_FIX)
            n = (exp > 0 ? exp + 1 : 1);
        else
            n = 3 + ndigs_exp;

        if (sign)
            n += 1;
        if (local_prec)
            n += local_prec + 1;
        else if (local_flags & PRINTF_FLAG_ALT_FORM)
            n += 1;

        local_width = local_width > n ? local_width - n : 0;

        if (!(local_flags & (PRINTF_FLAG_LEFT_ADJ | PRINTF_FLAG_ZERO_FILL))) {
            if (__printf_emit_fp_repeat(out, stream_len, ' ', (size_t) local_width) < 0)
                return -1;
            local_width = 0;
        }
        if (sign && __printf_emit_fp_char(out, stream_len, sign) < 0)
            return -1;

        if (!(local_flags & PRINTF_FLAG_LEFT_ADJ)) {
            if (__printf_emit_fp_repeat(out, stream_len, '0', (size_t) local_width) < 0)
                return -1;
            local_width = 0;
        }

        if (local_flags & PRINTF_FLAG_FLOAT_FIX) {
            char digit_out;

            n = exp > 0 ? exp : 0;
            do {
                if (n == -1 && __printf_emit_fp_char(out, stream_len, '.') < 0)
                    return -1;

                if (0 <= exp - n && exp - n < ndigs)
                    digit_out = dtoa->digits[exp - n];
                else
                    digit_out = '0';
                if (--n < -local_prec)
                    break;
                if (__printf_emit_fp_char(out, stream_len, (unsigned char) digit_out) < 0)
                    return -1;
            } while (1);
            if (__printf_emit_fp_char(out, stream_len, (unsigned char) digit_out) < 0)
                return -1;
            if ((local_flags & PRINTF_FLAG_ALT_FORM) && n == -1
                && __printf_emit_fp_char(out, stream_len, '.') < 0)
                return -1;
        } else {
            int pos;
            int digit_span_end;

            if (__printf_emit_fp_char(out, stream_len, (unsigned char) dtoa->digits[0]) < 0)
                return -1;
            if (local_prec > 0) {
                if (__printf_emit_fp_char(out, stream_len, '.') < 0)
                    return -1;
                digit_span_end = ndigs < 1 + local_prec ? ndigs : 1 + local_prec;
                if (digit_span_end > 1
                    && __printf_out_write(out, stream_len, &dtoa->digits[1],
                                          (size_t) (digit_span_end - 1))
                        < 0)
                    return -1;
                for (pos = digit_span_end; pos < 1 + local_prec; pos++) {
                    if (__printf_emit_fp_char(out, stream_len, '0') < 0)
                        return -1;
                }
            } else if (local_flags & PRINTF_FLAG_ALT_FORM) {
                if (__printf_emit_fp_char(out, stream_len, '.') < 0)
                    return -1;
            }

            {
                char expbuf[7];
                int expbuf_len = 0;

                expbuf[expbuf_len++] = (char) TOCASE_LOCAL(conv);
                sign = '+';
                if (exp < 0) {
                    exp = -exp;
                    sign = '-';
                }
                expbuf[expbuf_len++] = (char) sign;
                if (ndigs_exp >= 5)
                    expbuf[expbuf_len++] = (char) (exp / 10000 + '0');
                if (ndigs_exp >= 4)
                    expbuf[expbuf_len++] = (char) ((exp / 1000) % 10 + '0');
                if (ndigs_exp >= 3)
                    expbuf[expbuf_len++] = (char) ((exp / 100) % 10 + '0');
                expbuf[expbuf_len++] = (char) ((exp / 10) % 10 + '0');
                expbuf[expbuf_len++] = (char) ('0' + exp % 10);
                if (__printf_out_write(out, stream_len, expbuf, (size_t) expbuf_len) < 0)
                    return -1;
            }
        }
    }

    *flags = local_flags;
    *prec = local_prec;
    *width = local_width;
#undef TOCASE_LOCAL
    return 0;
}

/*
 * Shared helper for %a hexadecimal float formatting.
 */

int
__printf_float_format_hex(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                          int *width, unsigned char case_convert, va_list ap, struct dtoa *dtoa)
{
#define TOCASE_LOCAL(ch) ((ch) - case_convert)
    uint16_t local_flags = *flags | PRINTF_FLAG_FLOAT_EXP | PRINTF_FLAG_FLOAT_HEX;
    int local_prec = *prec;
    int local_width = *width;
    uint8_t sign;
    uint8_t ndigs;
    int exp;
    int n;

#if PRINTF_CAP_LONG_DOUBLE
    if ((local_flags & (PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE))
        == (PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE)) {
        PRINTF_LONG_DOUBLE_TYPE fval;

        fval = PRINTF_LONG_DOUBLE_ARG(ap);
        if (!(local_flags & PRINTF_FLAG_PRECISION))
            local_prec = -1;
        local_prec = __lfloat_x_engine(fval, dtoa, local_prec, case_convert);
        ndigs = local_prec + 1;
        exp = dtoa->exp;
    } else
#endif
    {
        FLOAT_UINT fval;

        fval = PRINTF_FLOAT_ARG(ap);
        if (!(local_flags & PRINTF_FLAG_PRECISION))
            local_prec = -1;

        ndigs = 1 + __float_x_engine(fval, dtoa, local_prec, case_convert);
        if (local_prec <= ndigs)
            local_prec = ndigs - 1;
        exp = dtoa->exp;
    }

    sign = 0;
    if (dtoa->flags & DTOA_MINUS)
        sign = '-';
    else if (local_flags & PRINTF_FLAG_PLUS)
        sign = '+';
    else if (local_flags & PRINTF_FLAG_SPACE)
        sign = ' ';

    if (dtoa->flags & (DTOA_NAN | DTOA_INF)) {
        char word[4];

        ndigs = sign ? 4 : 3;
        if (local_width > ndigs) {
            local_width -= ndigs;
            if (!(local_flags & PRINTF_FLAG_LEFT_ADJ)) {
                if (__printf_emit_fp_repeat(out, stream_len, ' ', (size_t) local_width) < 0)
                    return -1;
            }
        } else {
            local_width = 0;
        }
        if (sign && __printf_emit_fp_char(out, stream_len, sign) < 0)
            return -1;
        word[0] = (dtoa->flags & DTOA_NAN) ? 'n' : 'i';
        word[1] = (dtoa->flags & DTOA_NAN) ? 'a' : 'n';
        word[2] = (dtoa->flags & DTOA_NAN) ? 'n' : 'f';
        word[0] = (char) TOCASE_LOCAL(word[0]);
        word[1] = (char) TOCASE_LOCAL(word[1]);
        word[2] = (char) TOCASE_LOCAL(word[2]);
        if (__printf_out_write(out, stream_len, word, 3) < 0)
            return -1;
    } else {
        n = 3 + 2;
        if (sign)
            n += 1;
        if (local_prec)
            n += local_prec + 1;
        else if (local_flags & PRINTF_FLAG_ALT_FORM)
            n += 1;

        local_width = local_width > n ? local_width - n : 0;

        if (!(local_flags & (PRINTF_FLAG_LEFT_ADJ | PRINTF_FLAG_ZERO_FILL))) {
            if (__printf_emit_fp_repeat(out, stream_len, ' ', (size_t) local_width) < 0)
                return -1;
            local_width = 0;
        }
        if (sign && __printf_emit_fp_char(out, stream_len, sign) < 0)
            return -1;

        {
            char prefix[2] = { '0', (char) TOCASE_LOCAL('x') };

            if (__printf_out_write(out, stream_len, prefix, sizeof(prefix)) < 0)
                return -1;
        }

        if (!(local_flags & PRINTF_FLAG_LEFT_ADJ)) {
            if (__printf_emit_fp_repeat(out, stream_len, '0', (size_t) local_width) < 0)
                return -1;
            local_width = 0;
        }

        if (__printf_emit_fp_char(out, stream_len, (unsigned char) dtoa->digits[0]) < 0)
            return -1;
        if (local_prec > 0) {
            int pos;
            int digit_span_end;

            if (__printf_emit_fp_char(out, stream_len, '.') < 0)
                return -1;
            digit_span_end = ndigs < 1 + local_prec ? ndigs : 1 + local_prec;
            if (digit_span_end > 1
                && __printf_out_write(out, stream_len, &dtoa->digits[1],
                                      (size_t) (digit_span_end - 1)) < 0)
                return -1;
            for (pos = digit_span_end; pos < 1 + local_prec; pos++) {
                if (__printf_emit_fp_char(out, stream_len, '0') < 0)
                    return -1;
            }
        } else if (local_flags & PRINTF_FLAG_ALT_FORM) {
            if (__printf_emit_fp_char(out, stream_len, '.') < 0)
                return -1;
        }

        {
            char expbuf[4];
            int expbuf_len = 0;

            expbuf[expbuf_len++] = (char) TOCASE_LOCAL('p');
            sign = '+';
            if (exp < 0) {
                exp = -exp;
                sign = '-';
            }
            expbuf[expbuf_len++] = (char) sign;
            if (exp / 10)
                expbuf[expbuf_len++] = (char) (exp / 10 + '0');
            expbuf[expbuf_len++] = (char) ('0' + exp % 10);
            if (__printf_out_write(out, stream_len, expbuf, (size_t) expbuf_len) < 0)
                return -1;
        }
    }

    *flags = local_flags;
    *prec = local_prec;
    *width = local_width;
#undef TOCASE_LOCAL
    return 0;
}
