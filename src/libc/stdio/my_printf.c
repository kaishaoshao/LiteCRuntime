// my_printf.c (最终完整正确版 - 修复了所有回归BUG)
#include "my_printf.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if MY_PRINTF_SUPPORT_FLOAT
#include <math.h>
#endif
#if MY_PRINTF_SUPPORT_WIDE_CHAR
#include <wchar.h>
#endif

// --- 私有定义 ---
#define FLAG_LEFT_JUSTIFY (1 << 0)
#define FLAG_FORCE_SIGN (1 << 1)
#define FLAG_SPACE_SIGN (1 << 2)
#define FLAG_ALTERNATIVE (1 << 3)
#define FLAG_ZERO_PAD (1 << 4)
#define FLAG_THOUSANDS_SEP (1 << 5)
#define FLAG_CAPITALS (1 << 6)
#define FLAG_ENGINEERING (1 << 7)
#define FLAG_FMT_G (1 << 8)
#define FLAG_FMT_E (1 << 9)
#define FLAG_FMT_F (1 << 10)

#define LEN_MOD_NONE 0
#define LEN_MOD_HH 1
#define LEN_MOD_H 2
#define LEN_MOD_L 3
#define LEN_MOD_LL 4
#define LEN_MOD_J 5
#define LEN_MOD_Z 6
#define LEN_MOD_T 7

// --- 上下文与输出 ---
typedef struct my_printf_ctx {
  void (*_out_char)(void *p_user_data, char c);
  void *p_user_data;
  int char_count;
  char thousands_sep;
} my_printf_ctx_t;

static my_printf_custom_handler_t custom_handlers[256] = {NULL};

// --- 输出函数的具体实现 ---
static void _out_fn_file(void *p, char c) { fputc(c, (FILE *)p); }
typedef struct {
  char *buf;
  size_t rem;
} str_ctx_t;
static void _out_fn_string(void *p, char c) {
  str_ctx_t *s = (str_ctx_t *)p;
  if (s->rem > 1) {
    *(s->buf)++ = c;
    s->rem--;
  }
}
#if MY_PRINTF_SUPPORT_DYN_ALLOC
typedef struct {
  char **ptr;
  size_t count;
  size_t cap;
} alloc_ctx_t;
static void _out_fn_alloc(void *p, char c) {
  alloc_ctx_t *a = (alloc_ctx_t *)p;
  if (a->count >= a->cap) {
    size_t new_cap = (a->cap == 0) ? 64 : a->cap * 2;
    char *new_buf = (char *)realloc(*(a->ptr), new_cap);
    if (!new_buf) {
      free(*(a->ptr));
      *(a->ptr) = NULL;
      a->cap = 0;
      a->count = 0;
      return;
    }
    *(a->ptr) = new_buf;
    a->cap = new_cap;
  }
  if (*(a->ptr)) {
    (*(a->ptr))[a->count] = c;
    a->count++;
  }
}
#endif

// --- 内部辅助函数 ---
static const char DIGITS_LOWER[] = "0123456789abcdef";
static const char DIGITS_UPPER[] = "0123456789ABCDEF";
static void _out_char(my_printf_ctx_t *ctx, char c) {
  ctx->_out_char(ctx->p_user_data, c);
  ctx->char_count++;
}
static void _out_pad(my_printf_ctx_t *ctx, char pad_char, int n) {
  for (int i = 0; i < n; ++i)
    _out_char(ctx, pad_char);
}
static void _out_str(my_printf_ctx_t *ctx, const char *s, int len) {
  for (int i = 0; i < len; ++i)
    _out_char(ctx, s[i]);
}
static int _ultoa_rev(char *buf, unsigned long long val, int base,
                      const char *digits) {
  int i = 0;
  if (val == 0) {
    buf[i++] = '0';
  } else {
    while (val) {
      buf[i++] = digits[val % base];
      val /= base;
    }
  }
  return i;
}
static void _print_rev_num_str(my_printf_ctx_t *ctx, const char *buf, int len,
                               int flags) {
#if MY_PRINTF_SUPPORT_THOUSANDS_SEPARATOR
  if (flags & FLAG_THOUSANDS_SEP) {
    for (int i = len - 1; i >= 0; i--) {
      _out_char(ctx, buf[i]);
      if (i > 0 && (i % 3 == 0))
        _out_char(ctx, ctx->thousands_sep);
    }
    return;
  }
#endif
  for (int i = len - 1; i >= 0; i--)
    _out_char(ctx, buf[i]);
}

#if MY_PRINTF_SUPPORT_FLOAT
int _my_dtoa_engine(double val, char *buf, int precision, int flags) {
  char *p = buf;
  if (isnan(val)) {
    strcpy(buf, (flags & FLAG_CAPITALS) ? "NAN" : "nan");
    return 0;
  }
  if (isinf(val)) {
    strcpy(buf, (flags & FLAG_CAPITALS) ? "INF" : "inf");
    return 0;
  }
  char sign_char = 0;
  if (val < 0.0) {
    sign_char = '-';
    val = -val;
  } else if (flags & FLAG_FORCE_SIGN)
    sign_char = '+';
  else if (flags & FLAG_SPACE_SIGN)
    sign_char = ' ';
  if (sign_char)
    *p++ = sign_char;
  int exponent = (val == 0.0) ? 0 : floor(log10(val));
  char fmt_choice = 'f';
  if (flags & FLAG_ENGINEERING) {
    fmt_choice = 'e';
    if (val != 0.0) {
      int eng_exp = exponent - (exponent % 3);
      if (exponent < 0 && (exponent % 3 != 0))
        eng_exp -= 3;
      val /= pow(10.0, eng_exp);
      while (val >= 1000.0) {
        val /= 1000.0;
        eng_exp += 3;
      }
      exponent = eng_exp;
    }
  } else if (flags & FLAG_FMT_G) {
    if (precision < 0)
      precision = 6;
    if (exponent < -4 || exponent >= precision)
      fmt_choice = 'e';
    precision = (precision == 0) ? 1 : precision;
    if (fmt_choice == 'f')
      precision -= (exponent + 1);
    else
      precision--;
  } else if (flags & FLAG_FMT_E)
    fmt_choice = 'e';
  if (precision < 0)
    precision = 6;
  if (precision > 15)
    precision = 15;
  if (fmt_choice == 'e') {
    if (val != 0.0)
      val /= pow(10.0, exponent);
    val += 0.5 * pow(10.0, -precision);
    if (val >= 10.0) {
      val /= 10.0;
      exponent++;
    }
  } else {
    val += 0.5 * pow(10.0, -precision);
  }
  long long int_part = (long long)val;
  double frac_part = val - int_part;
  char temp[20];
  int i = 0;
  if (int_part == 0)
    temp[i++] = '0';
  else
    while (int_part > 0) {
      temp[i++] = (int_part % 10) + '0';
      int_part /= 10;
    }
  while (i-- > 0)
    *p++ = temp[i];
  char *frac_start = p;
  if (precision > 0 || (flags & FLAG_ALTERNATIVE))
    *p++ = '.';
  for (i = 0; i < precision; ++i) {
    frac_part *= 10.0;
    int digit = (int)frac_part;
    *p++ = digit + '0';
    frac_part -= digit;
  }
  if ((flags & FLAG_FMT_G) && !(flags & FLAG_ALTERNATIVE)) {
    char *end = p - 1;
    while (end > frac_start && *end == '0')
      *end-- = '\0', p--;
    if (end == frac_start && *end == '.')
      *end = '\0', p--;
  }
  if (fmt_choice == 'e') {
    *p++ = (flags & FLAG_CAPITALS) ? 'E' : 'e';
    *p++ = (exponent < 0) ? '-' : '+';
    if (exponent < 0)
      exponent = -exponent;
    if (exponent >= 100) {
      *p++ = (exponent / 100) + '0';
      exponent %= 100;
    }
    *p++ = (exponent / 10) + '0';
    *p++ = (exponent % 10) + '0';
  }
  *p = '\0';
  return 0;
}
#endif

// --- 核心格式化引擎 ---
int my_vfprintf_core(my_printf_ctx_t *ctx, const char *format, va_list ap) {
#ifdef MY_PRINTF_REENTRANT
  char num_buf[22];
#if MY_PRINTF_SUPPORT_FLOAT
  char float_buf[128];
#endif
#else
  static char num_buf[22];
#if MY_PRINTF_SUPPORT_FLOAT
  static char float_buf[128];
#endif
#endif
  ctx->char_count = 0;
  ctx->thousands_sep = ',';
  while (*format) {
    if (*format != '%') {
      _out_char(ctx, *format++);
      continue;
    }
    format++;
    int flags = 0, width = 0, precision = -1, len_mod = LEN_MOD_NONE;
    while (1) {
      char ch = *format;
      if (ch == '-')
        flags |= FLAG_LEFT_JUSTIFY;
      else if (ch == '+')
        flags |= FLAG_FORCE_SIGN;
      else if (ch == ' ')
        flags |= FLAG_SPACE_SIGN;
      else if (ch == '#')
        flags |= FLAG_ALTERNATIVE;
      else if (ch == '0')
        flags |= FLAG_ZERO_PAD;
#if MY_PRINTF_SUPPORT_THOUSANDS_SEPARATOR
      else if (ch == '\'')
        flags |= FLAG_THOUSANDS_SEP;
#endif
#if MY_PRINTF_SUPPORT_ENGINEERING_NOTATION
      else if (ch == '^')
        flags |= FLAG_ENGINEERING;
#endif
      else
        break;
      format++;
    }
    if (*format == '*') {
      width = va_arg(ap, int);
      format++;
    } else {
      while (*format >= '0' && *format <= '9')
        width = width * 10 + (*format++ - '0');
    }
    if (width < 0) {
      flags |= FLAG_LEFT_JUSTIFY;
      width = -width;
    }
    if (*format == '.') {
      format++;
      precision = 0;
      if (*format == '*') {
        precision = va_arg(ap, int);
        format++;
      } else {
        while (*format >= '0' && *format <= '9')
          precision = precision * 10 + (*format++ - '0');
      }
      if (precision < 0)
        precision = 0;
    }
    if (*format == 'l') {
      len_mod = LEN_MOD_L;
      format++;
      if (*format == 'l') {
        len_mod = LEN_MOD_LL;
        format++;
      }
    } else if (*format == 'h') {
      len_mod = LEN_MOD_H;
      format++;
      if (*format == 'h') {
        len_mod = LEN_MOD_HH;
        format++;
      }
    }
#if MY_PRINTF_SUPPORT_C99_LMODS
    else if (*format == 'j') {
      len_mod = LEN_MOD_J;
      format++;
    } else if (*format == 'z') {
      len_mod = LEN_MOD_Z;
      format++;
    } else if (*format == 't') {
      len_mod = LEN_MOD_T;
      format++;
    }
#endif
    char specifier = *format++;
    if (custom_handlers[(unsigned char)specifier]) {
      custom_handlers[(unsigned char)specifier](ctx, &ap, flags, width,
                                                precision);
      continue;
    }
    if (precision >= 0 && specifier != 'c' && specifier != 's' &&
        specifier != '%')
      flags &= ~FLAG_ZERO_PAD;

    switch (specifier) {
    case 'c': {
#if MY_PRINTF_SUPPORT_WIDE_CHAR
      if (len_mod == LEN_MOD_L) {
        char mb[MB_CUR_MAX];
        int l = wctomb(mb, va_arg(ap, wint_t));
        if (l > 0) {
          if (!(flags & FLAG_LEFT_JUSTIFY))
            _out_pad(ctx, ' ', width - l);
          _out_str(ctx, mb, l);
          if (flags & FLAG_LEFT_JUSTIFY)
            _out_pad(ctx, ' ', width - l);
        }
        break;
      }
#endif
      if (!(flags & FLAG_LEFT_JUSTIFY))
        _out_pad(ctx, ' ', width - 1);
      _out_char(ctx, (char)va_arg(ap, int));
      if (flags & FLAG_LEFT_JUSTIFY)
        _out_pad(ctx, ' ', width - 1);
      break;
    }
    case 's': {
#if MY_PRINTF_SUPPORT_WIDE_CHAR
      if (len_mod == LEN_MOD_L) {
        const wchar_t *wstr = va_arg(ap, const wchar_t *);
        if (!wstr)
          wstr = L"(null)";
        char mb_buf[MB_CUR_MAX];
        const wchar_t *p = wstr;
        int bytes_len = 0;
        while (*p && (precision < 0 || bytes_len < precision)) {
          int l = wctomb(mb_buf, *p++);
          if (l <= 0 || (precision >= 0 && bytes_len + l > precision))
            break;
          bytes_len += l;
        }
        if (!(flags & FLAG_LEFT_JUSTIFY))
          _out_pad(ctx, ' ', width - bytes_len);
        p = wstr;
        int bytes_to_write = bytes_len;
        while (bytes_to_write > 0) {
          int l = wctomb(mb_buf, *p++);
          if (l <= 0)
            break;
          _out_str(ctx, mb_buf, l);
          bytes_to_write -= l;
        }
        if (flags & FLAG_LEFT_JUSTIFY)
          _out_pad(ctx, ' ', width - bytes_len);
        break;
      }
#endif
      const char *s = va_arg(ap, const char *);
      if (!s)
        s = "(null)";
      int s_len = strnlen(s, (precision >= 0) ? precision : INT_MAX);
      if (!(flags & FLAG_LEFT_JUSTIFY))
        _out_pad(ctx, ' ', width - s_len);
      _out_str(ctx, s, s_len);
      if (flags & FLAG_LEFT_JUSTIFY)
        _out_pad(ctx, ' ', width - s_len);
      break;
    }
#if MY_PRINTF_SUPPORT_PERCENT_N
    case 'n': {
      void *p = va_arg(ap, void *);
      if (len_mod == LEN_MOD_LL)
        *(long long *)p = ctx->char_count;
      else if (len_mod == LEN_MOD_L)
        *(long *)p = ctx->char_count;
      else if (len_mod == LEN_MOD_H)
        *(short *)p = ctx->char_count;
      else if (len_mod == LEN_MOD_HH)
        *(signed char *)p = ctx->char_count;
#if MY_PRINTF_SUPPORT_C99_LMODS
      else if (len_mod == LEN_MOD_J)
        *(intmax_t *)p = ctx->char_count;
      else if (len_mod == LEN_MOD_Z)
        *(size_t *)p = ctx->char_count;
      else if (len_mod == LEN_MOD_T)
        *(ptrdiff_t *)p = ctx->char_count;
#endif
      else
        *(int *)p = ctx->char_count;
      break;
    }
#endif
    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
    case 'p': {
      unsigned long long u_val = 0;
      char prefix_c = 0;
      char alt_prefix[2] = {0};
      const char *digits = DIGITS_LOWER;
      int base = 10;
      if (specifier == 'd' || specifier == 'i') {
        long long val;
        if (len_mod == LEN_MOD_LL)
          val = va_arg(ap, long long);
        else if (len_mod == LEN_MOD_L)
          val = va_arg(ap, long);
        else
          val = va_arg(ap, int);
        if (val < 0) {
          prefix_c = '-';
          u_val = -val;
        } else {
          u_val = val;
          if (flags & FLAG_FORCE_SIGN)
            prefix_c = '+';
          else if (flags & FLAG_SPACE_SIGN)
            prefix_c = ' ';
        }
      } else {
        if (specifier == 'p') {
          u_val = (uintptr_t)va_arg(ap, void *);
          flags |= FLAG_ALTERNATIVE;
        } else if (len_mod == LEN_MOD_LL)
          u_val = va_arg(ap, unsigned long long);
        else if (len_mod == LEN_MOD_L)
          u_val = va_arg(ap, unsigned long);
        else
          u_val = va_arg(ap, unsigned int);
        if (specifier == 'X') {
          digits = DIGITS_UPPER;
          flags |= FLAG_CAPITALS;
        }
      }
      if (specifier == 'x' || specifier == 'p' || specifier == 'X') {
        base = 16;
        if (flags & FLAG_ALTERNATIVE && u_val != 0) {
          alt_prefix[0] = '0';
          alt_prefix[1] = (flags & FLAG_CAPITALS) ? 'X' : 'x';
        }
      } else if (specifier == 'o') {
        base = 8;
        if (flags & FLAG_ALTERNATIVE && (u_val != 0 || precision == 0)) {
          prefix_c = '0';
        }
      }
      int num_len = _ultoa_rev(num_buf, u_val, base, digits);
      if (prefix_c == '0' && u_val == 0 && precision > 0)
        prefix_c = 0;
      int eff_num_len = num_len;
      if (precision == 0 && u_val == 0)
        eff_num_len = 0;
      int prefix_len = (prefix_c ? 1 : 0) + (alt_prefix[0] ? 2 : 0);
      int thousands_sep_len = 0;
#if MY_PRINTF_SUPPORT_THOUSANDS_SEPARATOR
      if ((flags & FLAG_THOUSANDS_SEP) && base == 10)
        thousands_sep_len = (eff_num_len > 0) ? (eff_num_len - 1) / 3 : 0;
#endif
      int precision_pad = (precision > num_len) ? precision - num_len : 0;
      int total_len =
          eff_num_len + prefix_len + thousands_sep_len + precision_pad;
      int width_pad = (width > total_len) ? width - total_len : 0;
      char pad_char = (flags & FLAG_ZERO_PAD) ? '0' : ' ';
      if (!(flags & FLAG_LEFT_JUSTIFY)) {
        if (pad_char == '0') {
          if (prefix_c)
            _out_char(ctx, prefix_c);
          if (alt_prefix[0])
            _out_str(ctx, alt_prefix, 2);
          _out_pad(ctx, '0', width_pad);
        } else {
          _out_pad(ctx, ' ', width_pad);
          if (prefix_c)
            _out_char(ctx, prefix_c);
          if (alt_prefix[0])
            _out_str(ctx, alt_prefix, 2);
        }
      } else {
        if (prefix_c)
          _out_char(ctx, prefix_c);
        if (alt_prefix[0])
          _out_str(ctx, alt_prefix, 2);
      }
      _out_pad(ctx, '0', precision_pad);
      if (eff_num_len > 0)
        _print_rev_num_str(ctx, num_buf, num_len,
                           (base == 10) ? flags : flags & ~FLAG_THOUSANDS_SEP);
      if (flags & FLAG_LEFT_JUSTIFY)
        _out_pad(ctx, ' ', width_pad);
      break;
    }
#if MY_PRINTF_SUPPORT_FLOAT
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G': {
      int flt_flags = 0;
      if (specifier == 'g' || specifier == 'G')
        flt_flags |= FLAG_FMT_G;
      else if (specifier == 'e' || specifier == 'E')
        flt_flags |= FLAG_FMT_E;
      else
        flt_flags |= FLAG_FMT_F;
      if (specifier >= 'A' && specifier <= 'Z')
        flags |= FLAG_CAPITALS;
      _my_dtoa_engine(va_arg(ap, double), float_buf, precision,
                      flags | flt_flags);
      int float_len = strlen(float_buf);
      int width_pad = width > float_len ? width - float_len : 0;
      char pad_char = (flags & FLAG_ZERO_PAD) ? '0' : ' ';
      char sign_char =
          (float_buf[0] == '-' || float_buf[0] == '+' || float_buf[0] == ' ');
      if (!(flags & FLAG_LEFT_JUSTIFY)) {
        if (pad_char == '0' && sign_char)
          _out_char(ctx, float_buf[0]);
        _out_pad(ctx, (sign_char && pad_char == '0') ? '0' : ' ', width_pad);
        if (!sign_char || pad_char == ' ')
          _out_str(ctx, float_buf, float_len);
        else
          _out_str(ctx, float_buf + 1, float_len - 1);
      } else {
        _out_str(ctx, float_buf, float_len);
        _out_pad(ctx, ' ', width_pad);
      }
      break;
    }
#endif
    case '%':
      _out_char(ctx, '%');
      break;
    default:
      _out_char(ctx, '%');
      if (specifier)
        _out_char(ctx, specifier);
      else
        format--;
      break;
    }
  }
  return ctx->char_count;
}

// --- API 实现 ---
void my_printf_register_handler(char specifier,
                                my_printf_custom_handler_t handler) {
  if ((unsigned char)specifier < 256)
    custom_handlers[(unsigned char)specifier] = handler;
}
int my_vfprintf(FILE *stream, const char *format, va_list arg) {
  my_printf_ctx_t ctx = {._out_char = _out_fn_file, .p_user_data = stream};
  return my_vfprintf_core(&ctx, format, arg);
}
int my_vsnprintf(char *buffer, size_t count, const char *format, va_list arg) {
  my_printf_ctx_t ctx = {._out_char = _out_fn_string};
  str_ctx_t sctx = {.buf = buffer, .rem = count};
  if (count == 0)
    sctx.buf = NULL;
  ctx.p_user_data = &sctx;
  int ret = my_vfprintf_core(&ctx, format, arg);
  if (count > 0)
    *sctx.buf = '\0';
  return ret;
}
#if MY_PRINTF_SUPPORT_DYN_ALLOC
int my_vasprintf(char **strp, const char *format, va_list arg) {
  *strp = NULL;
  my_printf_ctx_t ctx = {._out_char = _out_fn_alloc};
  alloc_ctx_t actx = {.ptr = strp, .count = 0, .cap = 0};
  ctx.p_user_data = &actx;
  int ret = my_vfprintf_core(&ctx, format, arg);
  if (ret < 0 || (!*strp && ret > 0)) {
    free(*strp);
    *strp = NULL;
    return -1;
  }
  if (*strp) {
    (*strp)[actx.count] = '\0';
    char *fb = (char *)realloc(*strp, actx.count);
    if (fb)
      *strp = fb;
  }
  return ret;
}
#endif
int my_fprintf(FILE *stream, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int r = my_vfprintf(stream, format, ap);
  va_end(ap);
  return r;
}
int my_printf(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int r = my_vfprintf(stdout, format, ap);
  va_end(ap);
  return r;
}
int my_snprintf(char *buffer, size_t count, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int r = my_vsnprintf(buffer, count, format, ap);
  va_end(ap);
  return r;
}
#if MY_PRINTF_SUPPORT_DYN_ALLOC
int my_asprintf(char **strp, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int r = my_vasprintf(strp, format, ap);
  va_end(ap);
  return r;
}
#endif