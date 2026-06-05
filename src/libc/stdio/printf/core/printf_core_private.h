#ifndef _MICROCRT_PRINTF_CORE_PRIVATE_H_
#define _MICROCRT_PRINTF_CORE_PRIVATE_H_

/*
 * Core-private types and helpers for the MicroCRT printf framework.
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#include "printf_profile.h"
#include "printf_out_core.h"

#ifndef PRINTF_CAP_SHRINK
#define PRINTF_CAP_SHRINK 0
#endif

#ifndef PRINTF_CAP_SECURE
#define PRINTF_CAP_SECURE 0
#endif

#ifndef IO_VARIANT_IS_FLOAT
#define IO_VARIANT_IS_FLOAT(v) ((v) == __IO_VARIANT_FLOAT || (v) == __IO_VARIANT_DOUBLE)
#endif

#define TOLOWER(c) ((c) | ('a' - 'A'))

struct __printf_spec {
    uint16_t flags;
    int width;
    int prec;
    int argno;
};

static void
__printf_spec_reset(struct __printf_spec *spec)
{
    spec->flags = 0;
    spec->width = 0;
    spec->prec = 0;
    spec->argno = 0;
}

static int
__printf_emit(struct __printf_out *out, int *stream_len, unsigned ch)
{
    (*stream_len)++;
    return out->put((int) ch, out->cookie);
}

struct __printf_text_runtime {
    char *buf;
    wchar_t *wbuf;
#if PRINTF_CAP_WIDETOMB
    char *mb_buf;
#endif
};

struct __printf_core_runtime {
    char buf[PRINTF_BUF_SIZE];
#if PRINTF_CAP_WCHAR
    wchar_t wbuf[PRINTF_BUF_SIZE / 2];
#endif
#if PRINTF_CAP_WIDETOMB
    char mb[MB_LEN_MAX];
#endif
};

struct __printf_positional_state {
    va_list ap;
};

struct __printf_core_context;

#if PRINTF_CAP_POSITIONAL
static void __printf_positional_seek_arg(const CHAR *fmt,
                                         struct __printf_positional_state *state, int argno);
static void __printf_positional_init(struct __printf_core_context *ctx, va_list ap_orig);
static void __printf_positional_cleanup(struct __printf_core_context *ctx);
static void __printf_positional_rewind_arg(struct __printf_core_context *ctx, va_list ap_orig,
                                           int argno);
#else
static void
__printf_positional_init(struct __printf_core_context *ctx, va_list ap_orig)
{
    (void) ctx;
    (void) ap_orig;
}

static void
__printf_positional_cleanup(struct __printf_core_context *ctx)
{
    (void) ctx;
}

static void
__printf_positional_rewind_arg(struct __printf_core_context *ctx, va_list ap_orig, int argno)
{
    (void) ctx;
    (void) ap_orig;
    (void) argno;
}
#endif

struct __printf_core_context {
    int stream_len;
    const char *msg;
    struct __printf_text_runtime text;
#if PRINTF_CAP_POSITIONAL
    struct __printf_positional_state positional;
    const CHAR *fmt_orig;
#endif
};

static bool
__printf_cap_wchar_enabled(void)
{
    return PRINTF_CAP_WCHAR != 0;
}

static bool
__printf_cap_shrink_enabled(void)
{
    return PRINTF_CAP_SHRINK != 0;
}

static bool
__printf_cap_positional_enabled(void)
{
    return PRINTF_CAP_POSITIONAL != 0;
}

static bool
__printf_cap_c99_formats_enabled(void)
{
    return PRINTF_CAP_C99_FORMATS != 0;
}

#if PRINTF_CAP_BINARY
static bool
__printf_cap_binary_enabled(void)
{
    return PRINTF_CAP_BINARY != 0;
}
#endif

static bool
__printf_cap_secure_enabled(void)
{
    return PRINTF_CAP_SECURE != 0;
}

static bool
__printf_cap_percent_n_enabled(void)
{
    return PRINTF_CAP_PERCENT_N != 0;
}

static bool
__printf_use_wchar(uint16_t flags)
{
    return __printf_cap_wchar_enabled() && ((flags & PRINTF_FLAG_LONG) != 0);
}

static bool
__printf_spec_has_positional_arg(const struct __printf_spec *spec)
{
    return __printf_cap_positional_enabled() && spec->argno != 0;
}

static bool
__printf_conversion_is_float_family(unsigned char conv)
{
    unsigned char lower = TOLOWER(conv);

    if (lower >= 'e' && lower <= 'g')
        return true;
    return __printf_cap_c99_formats_enabled() && lower == 'a';
}

static bool
__printf_conversion_is_text_family(unsigned char conv)
{
    return conv == 'c' || conv == 's' || conv == 'n';
}

static bool
__printf_conversion_is_integer_family(unsigned char conv)
{
    switch (TOLOWER(conv)) {
    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'p':
        return true;
#if PRINTF_CAP_BINARY
    case 'b':
        return __printf_cap_binary_enabled();
#endif
    default:
        return false;
    }
}

static struct __printf_text_runtime
__printf_make_text_runtime(struct __printf_core_runtime *runtime)
{
    struct __printf_text_runtime text = {
        .buf = runtime->buf,
#if PRINTF_CAP_WCHAR
        .wbuf = runtime->wbuf,
#else
        .wbuf = NULL,
#endif
#if PRINTF_CAP_WIDETOMB
        .mb_buf = runtime->mb,
#endif
    };

    return text;
}

static void
__printf_core_context_init(struct __printf_core_context *ctx, struct __printf_core_runtime *runtime,
                           const CHAR *fmt)
{
    ctx->stream_len = 0;
    ctx->msg = NULL;
    ctx->text = __printf_make_text_runtime(runtime);
#if PRINTF_CAP_POSITIONAL
    ctx->fmt_orig = fmt;
#endif
}

#if PRINTF_CAP_POSITIONAL
static void
__printf_spec_reset_for_argno(struct __printf_spec *spec)
{
    spec->flags = 0;
    spec->width = 0;
    spec->prec = 0;
}
#endif

static void
__printf_spec_accumulate_digit(struct __printf_spec *spec, unsigned c)
{
    if (__printf_cap_shrink_enabled())
        return;

    c -= '0';
    if (spec->flags & PRINTF_FLAG_PRECISION)
        spec->prec = 10 * spec->prec + (int) c;
    else {
        spec->width = 10 * spec->width + (int) c;
        spec->flags |= PRINTF_FLAG_WIDTH;
    }
}

static void
__printf_spec_read_dynamic_field(struct __printf_spec *spec, va_list ap)
{
    if (__printf_cap_shrink_enabled()) {
        (void) va_arg(ap, int);
        return;
    }

    if (spec->flags & PRINTF_FLAG_PRECISION)
        spec->prec = va_arg(ap, int);
    else
        spec->width = va_arg(ap, int);
}

static void
__printf_spec_normalize_fields(struct __printf_spec *spec)
{
    if (__printf_cap_shrink_enabled())
        return;

    if (spec->prec < 0) {
        spec->prec = 0;
        spec->flags &= ~PRINTF_FLAG_PRECISION;
    }

    if (spec->width < 0) {
        spec->width = -spec->width;
        spec->flags |= PRINTF_FLAG_LEFT_ADJ;
    }
}

#ifdef PRINTF_FLAG_LONG
static uint16_t
__printf_apply_size_modifier(uint16_t flags, unsigned c)
{
    if (c == 'l') {
        if (flags & PRINTF_FLAG_LONG)
            flags |= PRINTF_FLAG_REPEAT_TYPE;
        return flags | PRINTF_FLAG_LONG;
    }

    if (c == 'h') {
        if (flags & PRINTF_FLAG_SHORT)
            flags |= PRINTF_FLAG_REPEAT_TYPE;
        return flags | PRINTF_FLAG_SHORT;
    }

    if (c == 'L')
        return flags | PRINTF_FLAG_REPEAT_TYPE | PRINTF_FLAG_LONG;

#if PRINTF_CAP_C99_FORMATS
    if (c == 'j') {
        if (sizeof(intmax_t) == sizeof(long))
            return flags | PRINTF_FLAG_LONG;
        if (sizeof(intmax_t) == sizeof(long long))
            return flags | PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE;
        if (sizeof(intmax_t) == sizeof(short))
            return flags | PRINTF_FLAG_SHORT;
        return flags;
    }

    if (c == 'z') {
        if (sizeof(size_t) == sizeof(long))
            return flags | PRINTF_FLAG_LONG;
        if (sizeof(size_t) == sizeof(long long))
            return flags | PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE;
        if (sizeof(size_t) == sizeof(short))
            return flags | PRINTF_FLAG_SHORT;
        return flags;
    }

    if (c == 't') {
        if (sizeof(ptrdiff_t) == sizeof(long))
            return flags | PRINTF_FLAG_LONG;
        if (sizeof(ptrdiff_t) == sizeof(long long))
            return flags | PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE;
        if (sizeof(ptrdiff_t) == sizeof(short))
            return flags | PRINTF_FLAG_SHORT;
        return flags;
    }
#endif

    return flags;
}
#endif

static const char *
__printf_validate_core_inputs(struct __printf_out *out, const char *fmt)
{
    if (!__printf_cap_secure_enabled())
        return NULL;
    if (out == NULL)
        return "output sink is null";
    if (fmt == NULL)
        return "null format string";
    return NULL;
}

static int
__printf_handle_core_error(struct __printf_out *out, const char *msg)
{
    if (!__printf_cap_secure_enabled())
        return -1;
#if PRINTF_CAP_SECURE
    if (__cur_handler != NULL)
        __cur_handler(msg, NULL, -1);
#else
    (void) msg;
#endif
    if (out != NULL)
        __printf_out_fail(out);
    return -1;
}

#if PRINTF_CAP_POSITIONAL || !IO_VARIANT_IS_FLOAT(PRINTF_CORE_PROFILE)
static void
__printf_skip_float_arg(uint16_t flags, va_list ap)
{
#if PRINTF_CAP_LONG_DOUBLE && PRINTF_FLOAT_CAP_LARGE
    if ((flags & (PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE))
        == (PRINTF_FLAG_LONG | PRINTF_FLAG_REPEAT_TYPE)) {
        (void) va_arg(ap, long double);
        return;
    }
    (void) va_arg(ap, double);
#elif PRINTF_CAP_FLOAT
    (void) flags;
    (void) va_arg(ap, uint32_t);
#else
    (void) flags;
    (void) va_arg(ap, double);
#endif
}
#endif

static ultoa_unsigned_t
__printf_read_unsigned_arg(va_list ap, uint16_t flags)
{
    ultoa_unsigned_t value;

    if (flags & PRINTF_FLAG_LONG) {
        if (flags & PRINTF_FLAG_REPEAT_TYPE)
#if PRINTF_CAP_LONG_LONG
            value = va_arg(ap, unsigned long long);
#else
            value = (unsigned long) va_arg(ap, unsigned long long);
#endif
        else
            value = va_arg(ap, unsigned long);
        return value;
    }

    value = va_arg(ap, unsigned int);
    if (flags & PRINTF_FLAG_SHORT) {
        if (flags & PRINTF_FLAG_REPEAT_TYPE)
            value = (unsigned char) value;
        else
            value = (unsigned short) value;
    }
    return value;
}

static ultoa_signed_t
__printf_read_signed_arg(va_list ap, uint16_t flags)
{
    ultoa_signed_t value;

    if (flags & PRINTF_FLAG_LONG) {
        if (flags & PRINTF_FLAG_REPEAT_TYPE)
#if PRINTF_CAP_LONG_LONG
            value = va_arg(ap, signed long long);
#else
            value = (signed long) va_arg(ap, signed long long);
#endif
        else
            value = va_arg(ap, signed long);
        return value;
    }

    value = va_arg(ap, signed int);
    if (flags & PRINTF_FLAG_SHORT) {
        if (flags & PRINTF_FLAG_REPEAT_TYPE)
            value = (signed char) value;
        else
            value = (signed short) value;
    }
    return value;
}

#endif
