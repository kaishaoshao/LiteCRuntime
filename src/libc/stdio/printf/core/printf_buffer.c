#include "printf_buffer.h"

/* Writes one character into a C-string sink. */
int
__printf_cstr_put(int ch, void *cookie)
{
    struct __printf_cstr_out *buf = (struct __printf_cstr_out *) cookie;

    if (buf->end == NULL || buf->pos < buf->end)
        *buf->pos++ = (char) ch;

    return (unsigned char) ch;
}

/* Writes a byte span into a C-string sink. */
int
__printf_cstr_write(const char *src, size_t len, void *cookie)
{
    struct __printf_cstr_out *buf = (struct __printf_cstr_out *) cookie;
    size_t writable = len;

    if (buf->end != NULL && buf->pos + writable > buf->end)
        writable = (size_t) (buf->end - buf->pos);

    if (writable != 0) {
        size_t i;

        for (i = 0; i < writable; ++i)
            buf->pos[i] = src[i];
        buf->pos += writable;
    }

    return (int) len;
}

/* Flushes a C-string sink. */
int
__printf_cstr_flush(void *cookie)
{
    (void) cookie;
    return 0;
}

/* Appends the terminating NUL for a C-string sink. */
int
__printf_cstr_finalize(void *cookie)
{
    struct __printf_cstr_out *buf = (struct __printf_cstr_out *) cookie;

    if (buf->cap != 0 && buf->pos != NULL)
        *buf->pos = '\0';
    return 0;
}

/* Acquires a C-string sink. */
void
__printf_cstr_lock(void *cookie)
{
    (void) cookie;
}

/* Releases a C-string sink. */
void
__printf_cstr_unlock(void *cookie)
{
    (void) cookie;
}

/* Reports whether a C-string sink accepts writes. */
int
__printf_cstr_writable(void *cookie)
{
    (void) cookie;
    return 1;
}

/* Marks a C-string sink as failed. */
void
__printf_cstr_mark_error(void *cookie)
{
    (void) cookie;
}

/* Binds a `struct __printf_out` to a C-string sink backend. */
void
__printf_buffer_init(struct __printf_out *out, struct __printf_cstr_out *buf)
{
    out->cookie = buf;
    out->put = __printf_cstr_put;
    out->write = __printf_cstr_write;
    out->flush = __printf_cstr_flush;
    out->finalize = __printf_cstr_finalize;
    out->lock = __printf_cstr_lock;
    out->unlock = __printf_cstr_unlock;
    out->writable = __printf_cstr_writable;
    out->mark_error = __printf_cstr_mark_error;
}

/* Runs a selected printf core against an unbounded C-string sink. */
int
__printf_buffer_route(char *s, const char *fmt, va_list ap,
                      int (*core)(struct __printf_out *out, const char *fmt, va_list ap))
{
    struct __printf_out out;
    struct __printf_cstr_out buf = { .pos = s, .end = NULL, .cap = (size_t) -1 };

    __printf_buffer_init(&out, &buf);
    return core(&out, fmt, ap);
}

/* Runs a selected printf core against a bounded C-string sink. */
int
__printf_buffer_route_n(char *s, size_t n, const char *fmt, va_list ap,
                        int (*core)(struct __printf_out *out, const char *fmt, va_list ap))
{
    struct __printf_out out;
    struct __printf_cstr_out buf;

    buf.pos = s;
    buf.end = n ? (s + n - 1) : s;
    buf.cap = n;

    __printf_buffer_init(&out, &buf);
    return core(&out, fmt, ap);
}
