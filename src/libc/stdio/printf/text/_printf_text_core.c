/*
 * Shared text formatting core for %c / %s family output.
 */

#if PRINTF_CAP_WIDETOMB || PRINTF_CAP_MBTOWIDE
#include <bits/types/mbstate_t.h>
#endif

#if PRINTF_CAP_WIDETOMB
static size_t
_mbslen(const wchar_t *s, size_t maxlen)
{
    mbstate_t ps = { 0 };
    wchar_t c;
    char tmp[MB_LEN_MAX];
    size_t len = 0;

    while (len < maxlen && (c = *s++) != L'\0') {
        int clen = __WCTOMB(tmp, c, &ps);

        if (clen == -1)
            return (size_t) clen;
        len += (size_t) clen;
    }

    return len;
}
#endif

#if PRINTF_CAP_MBTOWIDE
static size_t
_wcslen(const char *s, size_t maxlen)
{
    mbstate_t ps = { 0 };
    wchar_t c;
    size_t len = 0;

    while (len < maxlen && *s != '\0') {
        size_t clen = mbrtowc(&c, s, MB_LEN_MAX, &ps);

        if (c == L'\0')
            break;
        if (clen == (size_t)-1)
            return clen;

        s += clen;
        ++len;
    }

    return len;
}
#endif

static int
__printf_emit_repeat(struct __printf_out *out, int *stream_len, unsigned ch, size_t count)
{
    char chunk[16];

    memset(chunk, (int) ch, sizeof(chunk));

    while (count != 0) {
        size_t emit_len = count < sizeof(chunk) ? count : sizeof(chunk);

        if (__printf_out_write(out, stream_len, chunk, emit_len) < 0)
            return -1;
        count -= emit_len;
    }
    return 0;
}

static int
__printf_emit_width_tail(struct __printf_out *out, int *stream_len, int *width)
{
    if (*width > 0 && __printf_emit_repeat(out, stream_len, ' ', (size_t) *width) < 0)
        return -1;
    *width = 0;
    return 0;
}

static int
__printf_emit_narrow_span(struct __printf_out *out, int *stream_len, const char *p, size_t size)
{
    return __printf_out_write(out, stream_len, p, size);
}

static int
__printf_emit_reversed_span(struct __printf_out *out, int *stream_len, char *buf, size_t size)
{
    size_t left = 0;
    size_t right = size;

    while (left < right) {
        char tmp;

        right--;
        tmp = buf[left];
        buf[left] = buf[right];
        buf[right] = tmp;
        left++;
    }

    return __printf_out_write(out, stream_len, buf, size);
}

#if PRINTF_CAP_WCHAR
static int
__printf_emit_wide_span(struct __printf_out *out, int *stream_len, const wchar_t *wstr, size_t size
#if PRINTF_CAP_WIDETOMB
                        ,
                        char *mb_buf
#endif
)
{
#if PRINTF_CAP_WIDETOMB
    mbstate_t ps = { 0 };

    while (size) {
        wchar_t c = *wstr++;
        char *m = mb_buf;
        int mb_len = __WCTOMB(m, c, &ps);
        while (size && mb_len) {
            if (__printf_emit(out, stream_len, (unsigned char) *m++) < 0)
                return -1;
            size--;
            mb_len--;
        }
    }
#else
    while (size--) {
        if (__printf_emit(out, stream_len, (unsigned) *wstr++) < 0)
            return -1;
    }
#endif
    return 0;
}
#endif

static int
__printf_text_emit_common(struct __printf_out *out, int *stream_len, uint16_t flags, int *width,
                          size_t size, const char *pnt, const wchar_t *wstr,
                          struct __printf_text_runtime *text)
{
#if !PRINTF_CAP_SHRINK
    if (!(flags & PRINTF_FLAG_LEFT_ADJ)) {
        if ((size_t) *width > size) {
            if (__printf_emit_repeat(out, stream_len, ' ', (size_t) *width - size) < 0)
                return -1;
        }
    }
    *width -= (int) size;
#endif

    if (wstr) {
#if PRINTF_CAP_WCHAR
        return __printf_emit_wide_span(out, stream_len, wstr, size
#if PRINTF_CAP_WIDETOMB
                                       , text->mb_buf
#endif
        );
#else
        return -1;
#endif
    }
    return __printf_emit_narrow_span(out, stream_len, pnt, size);
}

/*
 * Shared formatter core for %c / %lc.
 */

static int
__printf_text_char(struct __printf_out *out, int *stream_len, uint16_t flags, int *width, int arg,
                   struct __printf_text_runtime *text)
{
#if PRINTF_CAP_SHRINK
    return __printf_emit(out, stream_len, (unsigned char) arg);
#else
    if (__printf_use_wchar(flags)) {
        text->wbuf[0] = (wchar_t) arg;
        text->wbuf[1] = L'\0';
        return __printf_text_emit_common(out, stream_len, flags, width, 1, NULL, text->wbuf,
                                         text);
    }
    text->buf[0] = (char) arg;
    return __printf_text_emit_common(out, stream_len, flags, width, 1, text->buf, NULL, text);
#endif
}

/*
 * Shared formatter core for %s / %ls.
 */

static int
__printf_text_string(struct __printf_out *out, int *stream_len, uint16_t flags, int *prec,
                     int *width, const char *pnt, const wchar_t *wstr,
                     struct __printf_text_runtime *text)
{
    size_t size;

    if (wstr != NULL) {
        size = (flags & PRINTF_FLAG_PRECISION) ? (size_t) *prec : SIZE_MAX;
#if PRINTF_CAP_WIDETOMB
        size = _mbslen(wstr, size);
        if (size == (size_t)-1)
            return -1;
#else
        size_t n = 0;

        while (n < size && wstr[n] != L'\0')
            ++n;
        size = n;
#endif
        return __printf_text_emit_common(out, stream_len, flags, width, size, NULL, wstr, text);
    }

#if PRINTF_CAP_SHRINK
    while (*pnt != '\0') {
        if (__printf_emit(out, stream_len, (unsigned char) *pnt++) < 0)
            return -1;
    }
    return 0;
#else
    size = (flags & PRINTF_FLAG_PRECISION) ? (size_t) *prec : SIZE_MAX;
#if PRINTF_CAP_MBTOWIDE
    size = _wcslen(pnt, size);
#else
    size = strnlen(pnt, size);
#endif
    return __printf_text_emit_common(out, stream_len, flags, width, size, pnt, NULL, text);
#endif
}
