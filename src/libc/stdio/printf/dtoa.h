/* Copyright © 2018, Keith Packard
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */

#ifndef _DTOA_H_
#define _DTOA_H_

#include "float/printf_float.h"
#include "../../libm/common/math_config.h"

#ifndef PRINTF_CAP_DOUBLE
#define PRINTF_CAP_DOUBLE 1
#endif

#ifndef PRINTF_CAP_FLOAT
#define PRINTF_CAP_FLOAT 0
#endif

#ifndef PRINTF_CAP_LONG_DOUBLE
#define PRINTF_CAP_LONG_DOUBLE 0
#endif

#ifndef PRINTF_CAP_INT128
#if PRINTF_CAP_LONG_DOUBLE && __SIZEOF_LONG_DOUBLE__ > 8
#define PRINTF_CAP_INT128 1
#else
#define PRINTF_CAP_INT128 0
#endif
#endif

/*
 * Resolved float-engine capabilities for this compiled profile.
 *
 * These macros are rebound below based on the active float/long-double
 * representation so the formatter core can stay representation-agnostic.
 */
#define PRINTF_FLOAT_CAP_32    0
#define PRINTF_FLOAT_CAP_64    0
#define PRINTF_FLOAT_CAP_LARGE 0

/* Shared decimal-exponent constants consumed by the dtoa engines. */
#define DTOA_MAX_EXP 1024

/* Default digit budgets until the representation-specific bindings below run. */
#define FLOAT_MAX_DIG      0
#define LONG_FLOAT_MAX_DIG 0

#include "printf_float_support.h"

/* Bind long-double formatting to the matching dtoa/dtox engine pair. */
#if PRINTF_CAP_LONG_DOUBLE
#if __SIZEOF_LONG_DOUBLE__ == 4
#undef PRINTF_FLOAT_CAP_32
#define PRINTF_FLOAT_CAP_32 1
#undef LONG_FLOAT_MAX_DIG
#define LONG_FLOAT_MAX_DIG     FTOA_MAX_DIG
#define __lfloat_d_engine      __ftoa_engine
#define __lfloat_x_engine      __ftox_engine
#define PRINTF_LONG_DOUBLE_ARG(ap) (asuint(va_arg(ap, long double)))
#define PRINTF_LONG_DOUBLE_TYPE uint32_t
#elif __SIZEOF_LONG_DOUBLE__ == 8
#undef PRINTF_FLOAT_CAP_64
#define PRINTF_FLOAT_CAP_64 1
#undef LONG_FLOAT_MAX_DIG
#define LONG_FLOAT_MAX_DIG     DTOA_MAX_DIG
#define __lfloat_d_engine      __dtoa_engine
#define __lfloat_x_engine      __dtox_engine
#define PRINTF_LONG_DOUBLE_ARG(ap) (asuint64(va_arg(ap, long double)))
#define PRINTF_LONG_DOUBLE_TYPE uint64_t
#elif __SIZEOF_LONG_DOUBLE__ > 8
#undef PRINTF_FLOAT_CAP_LARGE
#define PRINTF_FLOAT_CAP_LARGE 1
#undef LONG_FLOAT_MAX_DIG
#define LONG_FLOAT_MAX_DIG     LDTOA_MAX_DIG
#define __lfloat_d_engine      __ldtoa_engine
#define __lfloat_x_engine      __ldtox_engine
#define PRINTF_LONG_DOUBLE_ARG(ap) va_arg(ap, long double)
#define PRINTF_LONG_DOUBLE_TYPE long double
#endif
#endif

/* Bind the primary float path to the matching dtoa/dtox engine pair. */
#if PRINTF_CAP_DOUBLE
#if __SIZEOF_DOUBLE__ == 4
#undef PRINTF_FLOAT_CAP_32
#define PRINTF_FLOAT_CAP_32 1
#undef FLOAT_MAX_DIG
#define FLOAT_MAX_DIG        FTOA_MAX_DIG
#define __float_d_engine     __ftoa_engine
#define __float_x_engine     __ftox_engine
#define PRINTF_FLOAT_ARG(ap) (asuint(va_arg(ap, double)))
#elif __SIZEOF_DOUBLE__ == 8
#undef PRINTF_FLOAT_CAP_64
#define PRINTF_FLOAT_CAP_64 1
#undef FLOAT_MAX_DIG
#define FLOAT_MAX_DIG        DTOA_MAX_DIG
#define __float_d_engine     __dtoa_engine
#define __float_x_engine     __dtox_engine
#define PRINTF_FLOAT_ARG(ap) (asuint64(va_arg(ap, double)))
#endif
#endif

#if PRINTF_CAP_FLOAT
#undef PRINTF_FLOAT_CAP_32
#define PRINTF_FLOAT_CAP_32 1
#undef FLOAT_MAX_DIG
#define PRINTF_FLOAT_ARG(ap) (va_arg(ap, uint32_t))
#define FLOAT_MAX_DIG        FTOA_MAX_DIG
#define __float_d_engine     __ftoa_engine
#define __float_x_engine     __ftox_engine
#endif

/* Engine declarations gated by the resolved float capability set. */
#if PRINTF_FLOAT_CAP_LARGE
int __ldtoa_engine(long double x, struct dtoa *dtoa, int max_digits, bool fmode, int max_decimals);

int __ldtox_engine(long double x, struct dtoa *dtoa, int prec, unsigned char case_convert);
#endif

#if PRINTF_FLOAT_CAP_64
int __dtoa_engine(uint64_t x, struct dtoa *dtoa, int max_digits, bool fmode, int max_decimals);

int __dtox_engine(uint64_t x, struct dtoa *dtoa, int prec, unsigned char case_convert);

FLOAT64
__atod_engine(uint64_t m10, int e10);
#endif

#if PRINTF_FLOAT_CAP_32
int   __ftoa_engine(uint32_t val, struct dtoa *ftoa, int max_digits, bool fmode, int max_decimals);

int   __ftox_engine(uint32_t x, struct dtoa *dtoa, int prec, unsigned char case_convert);

float __atof_engine(uint32_t m10, int e10);
#endif

#endif /* !_DTOA_H_ */
