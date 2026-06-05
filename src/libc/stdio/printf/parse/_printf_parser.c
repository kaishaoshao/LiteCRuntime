/*
 * Parser scan/accept/finalize helpers used directly by the core loop.
 *
 * This file owns token-level parser progression:
 * - literal scanning
 * - flag/width/precision/size acceptance
 * - final normalization of one parsed conversion spec
 */

#include "printf_core_private.h"

#define __PRINTF_PARSE_SPEC_OK   0
#define __PRINTF_PARSE_SPEC_DONE 1

static int
__printf_parse_scan_literal(struct __printf_out *out, int *stream_len, const CHAR **fmt, unsigned *c)
{
    for (;;) {
        *c = *(*fmt)++;
        if (!*c)
            return 0;
        if (*c == '%') {
            *c = *(*fmt)++;
            if (*c != '%')
                return 1;
        }
        if (__printf_emit(out, stream_len, *c) < 0)
            return -1;
    }
}

static void
__printf_parse_finalize_spec(struct __printf_spec *spec)
{
    __printf_spec_normalize_fields(spec);
}

static int
__printf_parse_accept_flag_char(struct __printf_spec *spec, unsigned c)
{
    if (spec->flags >= PRINTF_FLAG_WIDTH)
        return 0;

    switch (c) {
    case '0':
        spec->flags |= PRINTF_FLAG_ZERO_FILL;
        return 1;
    case '+':
        spec->flags |= PRINTF_FLAG_PLUS;
        __fallthrough;
    case ' ':
        spec->flags |= PRINTF_FLAG_SPACE;
        return 1;
    case '-':
        spec->flags |= PRINTF_FLAG_LEFT_ADJ;
        return 1;
    case '#':
        spec->flags |= PRINTF_FLAG_ALT_FORM;
        return 1;
    case '\'':
        /* C/POSIX locale keeps thousands_sep empty for this implementation. */
        return 1;
    default:
        return 0;
    }
}

static int
__printf_parse_accept_digit_char(struct __printf_spec *spec, unsigned c)
{
    if (spec->flags >= PRINTF_FLAG_LONG || c < '0' || c > '9')
        return 0;

    __printf_spec_accumulate_digit(spec, c);
    return 1;
}

static int
__printf_parse_accept_dynamic_field(struct __printf_spec *spec, va_list ap)
{
    if (spec->flags >= PRINTF_FLAG_LONG)
        return 0;

    __printf_spec_read_dynamic_field(spec, ap);

    if (!(spec->flags & PRINTF_FLAG_PRECISION))
        spec->flags |= PRINTF_FLAG_WIDTH;
    return 1;
}

static int
__printf_parse_accept_precision_marker(struct __printf_spec *spec)
{
    if (spec->flags >= PRINTF_FLAG_LONG)
        return 0;
    if (spec->flags & PRINTF_FLAG_PRECISION)
        return -1;

    spec->flags |= PRINTF_FLAG_PRECISION;
    return 1;
}

static int
__printf_parse_accept_size_modifier(struct __printf_spec *spec, unsigned c)
{
    uint16_t flags = __printf_apply_size_modifier(spec->flags, c);

    if (flags == spec->flags)
        return 0;

    spec->flags = flags;
    return 1;
}

#if PRINTF_CAP_POSITIONAL
static int
__printf_parse_accept_positional_marker(struct __printf_spec *spec, unsigned c, const CHAR *fmt_orig,
                                        struct __printf_positional_state *state, va_list ap_orig)
{
    if (!__printf_cap_positional_enabled() || spec->flags >= PRINTF_FLAG_LONG || c != '$')
        return 0;

#if PRINTF_CAP_POSITIONAL
    if (spec->argno) {
        va_end(state->ap);
        va_copy(state->ap, ap_orig);
        __printf_positional_seek_arg(fmt_orig, state,
                                     (spec->flags & PRINTF_FLAG_PRECISION) ? spec->prec
                                                                           : spec->width);
        if (spec->flags & PRINTF_FLAG_PRECISION)
            spec->prec = va_arg(state->ap, int);
        else
            spec->width = va_arg(state->ap, int);
    } else {
        spec->argno = spec->width;
        __printf_spec_reset_for_argno(spec);
    }
#else
    (void) fmt_orig;
    (void) state;
    (void) ap_orig;
#endif

    return 1;
}
#endif

static int
__printf_parse_conversion_spec(struct __printf_spec *spec, unsigned *conv, const CHAR **fmt,
                               struct __printf_core_context *ctx, va_list ap_orig)
{
    unsigned c = *conv;

    __printf_spec_reset(spec);

    do {
        if (__printf_parse_accept_flag_char(spec, c))
            continue;

        if (spec->flags < PRINTF_FLAG_LONG) {
            if (__printf_parse_accept_digit_char(spec, c))
                continue;
            if (c == '*') {
                /*
                 * Positional args must be used together, so wait
                 * for the value to appear before dealing with
                 * width and precision fields.
                 */
                if (__printf_spec_has_positional_arg(spec))
                    continue;
                (void) __printf_parse_accept_dynamic_field(spec,
#if PRINTF_CAP_POSITIONAL
                                                           ctx->positional.ap
#else
                                                           ap_orig
#endif
                );
                continue;
            }
            if (c == '.') {
                if (__printf_parse_accept_precision_marker(spec) < 0)
                    return __PRINTF_PARSE_SPEC_DONE;
                continue;
            }
#if PRINTF_CAP_POSITIONAL
            if (__printf_parse_accept_positional_marker(spec, c, ctx->fmt_orig, &ctx->positional,
                                                        ap_orig))
                continue;
#endif
        }

        if (__printf_parse_accept_size_modifier(spec, c))
            continue;

        break;
    } while ((c = *(*fmt)++) != 0);

    *conv = c;

    if (__printf_spec_has_positional_arg(spec))
        __printf_positional_rewind_arg(ctx, ap_orig, spec->argno);

    __printf_parse_finalize_spec(spec);
    return __PRINTF_PARSE_SPEC_OK;
}
