/*
 * Shared family dispatcher for one parsed conversion descriptor.
 *
 * The core only knows four phases:
 * 1. parse one conversion spec
 * 2. select one family
 * 3. run that family's entry/helper chain
 * 4. let the backend consume the emitted bytes
 *
 * This file therefore acts as the bridge from `printf_core_*` to the
 * text/int/float families. That keeps profile logic centralized at the core
 * level instead of scattering it through individual helpers.
 */

#if IO_VARIANT_IS_FLOAT(PRINTF_CORE_PROFILE)
#include "float/printf_float.h"
#endif

#define __PRINTF_DISPATCH_ERROR       (-1)
#define __PRINTF_DISPATCH_OK          0
#define __PRINTF_DISPATCH_SKIP_TAIL   1
#define __PRINTF_DISPATCH_SECURE_FAIL 2

static int
__printf_emit_unknown_conversion(struct __printf_out *out, int *stream_len, unsigned char conv)
{
    char unknown[2];

    unknown[0] = '%';
    unknown[1] = (char) conv;
    if (__printf_out_write(out, stream_len, unknown, sizeof(unknown)) < 0)
        return -1;
    return 0;
}

static int
__printf_dispatch_float_family(struct __printf_out *out, int *stream_len, uint16_t *flags,
                               int *prec, int *width, unsigned char conv, va_list ap,
                               struct __printf_text_runtime *text
#if IO_VARIANT_IS_FLOAT(PRINTF_CORE_PROFILE)
                               ,
                               struct dtoa *dtoa
#endif
)
{
    if (!__printf_conversion_is_float_family(conv))
        return __PRINTF_DISPATCH_SKIP_TAIL;

#if !PRINTF_CAP_SHRINK
#if IO_VARIANT_IS_FLOAT(PRINTF_CORE_PROFILE)
    switch (TOLOWER(conv)) {
    case 'f':
        return __printf_float_f(out, stream_len, flags, prec, width, conv, ap, dtoa);
    case 'e':
        return __printf_float_e(out, stream_len, flags, prec, width, conv, ap, dtoa);
    case 'g':
        return __printf_float_g(out, stream_len, flags, prec, width, conv, ap, dtoa);
#if PRINTF_CAP_C99_FORMATS
    case 'a':
        if (__printf_cap_c99_formats_enabled())
            return __printf_float_a(out, stream_len, flags, prec, width, conv, ap, dtoa);
        return __PRINTF_DISPATCH_SKIP_TAIL;
#endif
    default:
        return __PRINTF_DISPATCH_SKIP_TAIL;
    }
#elif PRINTF_CAP_FLOAT_PLACEHOLDER
    __printf_skip_float_arg(*flags, ap);
    if (__printf_text_emit_common(out, stream_len, *flags, width, sizeof("*float*") - 1,
                                  "*float*", NULL, text)
        < 0)
        return __PRINTF_DISPATCH_ERROR;
    return __PRINTF_DISPATCH_OK;
#else
    (void) out;
    (void) stream_len;
    (void) flags;
    (void) prec;
    (void) width;
    (void) conv;
    (void) ap;
    (void) text;
    return __PRINTF_DISPATCH_SKIP_TAIL;
#endif
#else
    (void) out;
    (void) stream_len;
    (void) flags;
    (void) prec;
    (void) width;
    (void) conv;
    (void) ap;
    (void) text;
    return __PRINTF_DISPATCH_SKIP_TAIL;
#endif
}

static int
__printf_dispatch_text_family(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                              int *width, unsigned char conv, va_list ap, const char **msg_out,
                              struct __printf_text_runtime *text)
{
    int dispatch_ret;

    if (!__printf_conversion_is_text_family(conv))
        return __PRINTF_DISPATCH_SKIP_TAIL;

    switch (conv) {
    case 'c':
        if (__printf_text_char_entry(out, stream_len, *flags, width, ap, text) < 0)
            return __PRINTF_DISPATCH_ERROR;
        return __PRINTF_DISPATCH_OK;

    case 's':
        dispatch_ret = __printf_text_string_entry(out, stream_len, *flags, prec, width, ap, msg_out,
                                                  text);
        if (dispatch_ret < 0)
            return __PRINTF_DISPATCH_ERROR;
        if (__printf_cap_secure_enabled() && dispatch_ret > 0)
            return __PRINTF_DISPATCH_SECURE_FAIL;
        return __PRINTF_DISPATCH_OK;

    case 'n':
        if (!__printf_cap_percent_n_enabled() && !__printf_cap_secure_enabled())
            return __PRINTF_DISPATCH_SKIP_TAIL;

        dispatch_ret = __printf_text_percent_n_checked(*flags, ap, *stream_len, msg_out);
        if (dispatch_ret < 0)
            return __PRINTF_DISPATCH_ERROR;
        if (__printf_cap_secure_enabled() && dispatch_ret > 0)
            return __PRINTF_DISPATCH_SECURE_FAIL;
        return __PRINTF_DISPATCH_OK;

    default:
        return __PRINTF_DISPATCH_SKIP_TAIL;
    }
}

static int
__printf_dispatch_int_family(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                             int *width, unsigned char conv, va_list ap, char *buf)
{
    if (!__printf_conversion_is_integer_family(conv))
        return __PRINTF_DISPATCH_SKIP_TAIL;

    switch (TOLOWER(conv)) {
    case 'd':
    case 'i':
        if (__printf_int_dec_entry(out, stream_len, flags, prec, width, ap, buf) < 0)
            return __PRINTF_DISPATCH_ERROR;
        return __PRINTF_DISPATCH_OK;
    case 'u':
        if (__printf_int_udec_entry(out, stream_len, flags, prec, width, ap, buf) < 0)
            return __PRINTF_DISPATCH_ERROR;
        return __PRINTF_DISPATCH_OK;
    case 'o':
        if (__printf_int_oct_entry(out, stream_len, flags, prec, width, ap, buf) < 0)
            return __PRINTF_DISPATCH_ERROR;
        return __PRINTF_DISPATCH_OK;
    case 'x':
        if (__printf_int_hex_entry(out, stream_len, flags, prec, width, conv, ap, buf)
            < 0)
            return __PRINTF_DISPATCH_ERROR;
        return __PRINTF_DISPATCH_OK;
    case 'p':
        if (__printf_int_pointer_entry(out, stream_len, flags, prec, width, ap, buf) < 0)
            return __PRINTF_DISPATCH_ERROR;
        return __PRINTF_DISPATCH_OK;
#if PRINTF_CAP_BINARY
    case 'b':
        if (__printf_int_bin_entry(out, stream_len, flags, prec, width, conv, ap, buf)
            < 0)
            return __PRINTF_DISPATCH_ERROR;
        return __PRINTF_DISPATCH_OK;
#endif
    default:
        return __PRINTF_DISPATCH_SKIP_TAIL;
    }
}

static int
__printf_dispatch_conversion(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                             int *width, unsigned char conv, va_list ap,
                             struct __printf_text_runtime *text
#if IO_VARIANT_IS_FLOAT(PRINTF_CORE_PROFILE)
                             ,
                             struct dtoa *dtoa
#endif
                             ,
                             const char **msg_out
)
{
    int dispatch_status;

    dispatch_status = __printf_dispatch_text_family(out, stream_len, flags, prec, width, conv, ap,
                                                    msg_out, text);
    if (dispatch_status != __PRINTF_DISPATCH_SKIP_TAIL)
        return dispatch_status;

    dispatch_status = __printf_dispatch_int_family(out, stream_len, flags, prec, width, conv, ap,
                                                   text->buf);
    if (dispatch_status != __PRINTF_DISPATCH_SKIP_TAIL)
        return dispatch_status;

    dispatch_status = __printf_dispatch_float_family(out, stream_len, flags, prec, width, conv, ap,
                                                     text
#if IO_VARIANT_IS_FLOAT(PRINTF_CORE_PROFILE)
                                                     ,
                                                     dtoa
#endif
    );
    if (dispatch_status != __PRINTF_DISPATCH_SKIP_TAIL)
        return dispatch_status;

    if (__printf_emit_unknown_conversion(out, stream_len, conv) < 0)
        return __PRINTF_DISPATCH_ERROR;
    return __PRINTF_DISPATCH_SKIP_TAIL;
}
