#ifndef _MICROCRT_PRINTF_FLOAT_H_
#define _MICROCRT_PRINTF_FLOAT_H_

#include <stdio.h>
#include <stdint.h>

#include "../core/printf_profile.h"
#include "../core/printf_out_core.h"

#define DTOA_MINUS 1
#define DTOA_ZERO  2
#define DTOA_INF   4
#define DTOA_NAN   8

#if __LDBL_MANT_DIG__ == 113
#define LDTOA_MAX_DIG 34
#elif __LDBL_MANT_DIG__ == 106
#define LDTOA_MAX_DIG 32
#elif __LDBL_MANT_DIG__ == 64
#define LDTOA_MAX_DIG 20
#endif

#define DTOA_MAX_DIG 17
#define FTOA_MAX_DIG 9

#define PRINTF_LOCAL_MAX(a, b) \
    ({                         \
        __typeof(a) _a = a;    \
        __typeof(b) _b = b;    \
        _a > _b ? _a : _b;     \
    })

#define PRINTF_LOCAL_MIN(a, b) \
    ({                         \
        __typeof(a) _a = a;    \
        __typeof(b) _b = b;    \
        _a < _b ? _a : _b;     \
    })

#if PRINTF_CAP_LONG_DOUBLE
#if __SIZEOF_LONG_DOUBLE__ > 8
#define DTOA_DIGITS LDTOA_MAX_DIG
#elif __SIZEOF_LONG_DOUBLE__ == 8
#define DTOA_DIGITS DTOA_MAX_DIG
#elif __SIZEOF_LONG_DOUBLE__ == 4
#define DTOA_DIGITS FTOA_MAX_DIG
#endif
#elif PRINTF_CAP_DOUBLE
#if __SIZEOF_DOUBLE__ == 8
#define DTOA_DIGITS DTOA_MAX_DIG
#elif __SIZEOF_DOUBLE__ == 4
#define DTOA_DIGITS FTOA_MAX_DIG
#endif
#elif PRINTF_CAP_FLOAT
#define DTOA_DIGITS FTOA_MAX_DIG
#endif

struct dtoa {
    int32_t exp;
    uint8_t flags;
    char    digits[DTOA_DIGITS];
};

/* Formats `%f`, `%e` and `%g` style decimal floating-point conversions. */
int __printf_float_format_dec(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                              int *width, unsigned char conv, unsigned char case_convert, va_list ap,
                              struct dtoa *dtoa);

/* Formats `%a` style hexadecimal floating-point conversions. */
int __printf_float_format_hex(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                              int *width, unsigned char case_convert, va_list ap, struct dtoa *dtoa);

/* Dispatches a `%f` conversion through the decimal float backend. */
int __printf_float_f(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                     int *width, unsigned char conv, va_list ap, struct dtoa *dtoa);

/* Dispatches a `%e` conversion through the decimal float backend. */
int __printf_float_e(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                     int *width, unsigned char conv, va_list ap, struct dtoa *dtoa);

/* Dispatches a `%g` conversion through the decimal float backend. */
int __printf_float_g(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                     int *width, unsigned char conv, va_list ap, struct dtoa *dtoa);

#if PRINTF_CAP_C99_FORMATS
/* Dispatches a `%a` conversion through the hexadecimal float backend. */
int __printf_float_a(struct __printf_out *out, int *stream_len, uint16_t *flags, int *prec,
                     int *width, unsigned char conv, va_list ap, struct dtoa *dtoa);
#endif

#endif
