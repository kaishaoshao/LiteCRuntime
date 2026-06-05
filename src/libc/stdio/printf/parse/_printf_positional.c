/*
 * Positional-argument lifecycle helpers for one core invocation.
 *
 * This file owns positional state management:
 * - init/cleanup of the saved va_list
 * - seek/rewind to a target argument index
 */

#include "printf_core_private.h"

/*
 * Repeatedly scan the format string until the argument vector points at
 * target_argno so the outer parser/dispatcher can consume the desired arg.
 */
static void
__printf_positional_seek_arg(const CHAR *fmt_orig, struct __printf_positional_state *state,
                             int target_argno)
{
    unsigned    c;
    uint16_t    flags;
    int         current_argno = 1;
    int         argno;
    int         width;
    const CHAR *fmt = fmt_orig;

    while (current_argno < target_argno) {
        for (;;) {
            c = *fmt++;
            if (c == '\0')
                return;
            if (c == '%') {
                c = *fmt++;
                if (c != '%')
                    break;
            }
        }

        flags = 0;
        width = 0;
        argno = 0;

        do {
            uint16_t size_flags;

            if (flags < PRINTF_FLAG_WIDTH) {
                switch (c) {
                case '0':
                case '+':
                case ' ':
                case '-':
                case '#':
                case '\'':
                    continue;
                }
            }

            if (flags < PRINTF_FLAG_LONG) {
                if (c >= '0' && c <= '9') {
                    c -= '0';
                    width = 10 * width + (int) c;
                    flags |= PRINTF_FLAG_WIDTH;
                    continue;
                }

                if (c == '$') {
                    if (argno != 0) {
                        if (width == current_argno) {
                            c = 'c';
                            argno = width;
                            break;
                        }
                    } else {
                        argno = width;
                    }
                    width = 0;
                    continue;
                }

                if (c == '*' || c == '.') {
                    width = 0;
                    continue;
                }
            }

            size_flags = __printf_apply_size_modifier(flags, c);
            if (size_flags != flags) {
                flags = size_flags;
                continue;
            }

            break;
        } while ((c = *fmt++) != '\0');

        if (argno == 0)
            break;

        if (argno == current_argno) {
            if (__printf_conversion_is_float_family((unsigned char) c)) {
                __printf_skip_float_arg(flags, state->ap);
            } else if (c == 'c') {
                (void) va_arg(state->ap, int);
            } else if (c == 's') {
                (void) va_arg(state->ap, char *);
            } else if (c == 'd' || c == 'i') {
                (void) __printf_read_signed_arg(state->ap, flags);
            } else {
                (void) __printf_read_unsigned_arg(state->ap, flags);
            }

            ++current_argno;
            fmt = fmt_orig;
        }
    }
}

static void
__printf_positional_init(struct __printf_core_context *ctx, va_list ap_orig)
{
    va_copy(ctx->positional.ap, ap_orig);
}

static void
__printf_positional_cleanup(struct __printf_core_context *ctx)
{
    va_end(ctx->positional.ap);
}

static void
__printf_positional_rewind_arg(struct __printf_core_context *ctx, va_list ap_orig, int argno)
{
    va_end(ctx->positional.ap);
    va_copy(ctx->positional.ap, ap_orig);
    __printf_positional_seek_arg(ctx->fmt_orig, &ctx->positional, argno);
}
