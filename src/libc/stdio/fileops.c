#include "printf/core/printf_file.h"

void
__flockfile(FILE *f)
{
#ifdef __STDIO_LOCKING
    if (f->lock != __LOCK_NONE)
        __lock_acquire_recursive(f->lock);
#else
    (void) f;
#endif
}

void
__funlockfile(FILE *f)
{
#ifdef __STDIO_LOCKING
    if (f->lock != __LOCK_NONE)
        __lock_release_recursive(f->lock);
#else
    (void) f;
#endif
}

int
__printf_file_put(int ch, void *cookie)
{
    FILE *stream = (FILE *) cookie;
    return stream->put((char) ch, stream);
}

int
__printf_file_write(const char *buf, size_t len, void *cookie)
{
    FILE *stream = (FILE *) cookie;

    if (stream->write != NULL)
        return stream->write(buf, len, stream);

    for (size_t written = 0; written < len; ++written) {
        if (stream->put(buf[written], stream) < 0)
            return -1;
    }
    return (int) len;
}

int
__printf_file_flush(void *cookie)
{
    FILE *stream = (FILE *) cookie;
    if (stream->flush == NULL)
        return 0;
    return stream->flush(stream);
}

int
__printf_file_finalize(void *cookie)
{
#if MICROCRT_PRINTF_ENABLE_AUTO_FLUSH
    return __printf_file_flush(cookie);
#else
    (void) cookie;
    return 0;
#endif
}

void
__printf_file_lock(void *cookie)
{
    __flockfile((FILE *) cookie);
}

void
__printf_file_unlock(void *cookie)
{
    __funlockfile((FILE *) cookie);
}

int
__printf_file_writable(void *cookie)
{
    FILE *stream = (FILE *) cookie;
    return (stream->flags & __SWR) != 0;
}

void
__printf_file_mark_error(void *cookie)
{
    FILE *stream = (FILE *) cookie;
    stream->flags |= __SERR;
}

void
__printf_file_init(struct __printf_out *out, FILE *stream)
{
    out->cookie = stream;
    out->put = __printf_file_put;
    out->write = __printf_file_write;
    out->flush = __printf_file_flush;
    out->finalize = __printf_file_finalize;
    out->lock = __printf_file_lock;
    out->unlock = __printf_file_unlock;
    out->writable = __printf_file_writable;
    out->mark_error = __printf_file_mark_error;
}

int
__printf_file_route(FILE *stream, const char *fmt, va_list ap,
                    int (*core)(struct __printf_out *out, const char *fmt, va_list ap))
{
    struct __printf_out out;

    __printf_file_init(&out, stream);
    return core(&out, fmt, ap);
}

static int
__file_write_direct(FILE *stream, const char *buf, size_t len)
{
    if (stream->write != NULL)
        return stream->write(buf, len, stream);

    for (size_t i = 0; i < len; ++i) {
        if (stream->put((char) buf[i], stream) < 0)
            return -1;
    }
    return (int) len;
}

static int
__file_flush_wbuf(FILE *stream)
{
    int ret;

    if (stream->wbuf == NULL || stream->wbuf_len == 0)
        return 0;

    ret = __file_write_direct(stream, (const char *) stream->wbuf, stream->wbuf_len);
    if (ret < 0 || (size_t) ret != stream->wbuf_len) {
        stream->flags |= __SERR;
        return EOF;
    }

    stream->wbuf_len = 0;
    return 0;
}

int
__file_str_put(char c, FILE *stream)
{
    struct __file_str *sstream = (struct __file_str *) stream;

    if (sstream->pos != sstream->end)
        *sstream->pos++ = c;

    return (unsigned char) c;
}

int
__file_str_put_alloc(char c, FILE *stream)
{
    struct __file_str *sstream = (struct __file_str *) stream;

    if (sstream->pos == sstream->end)
        return EOF;

    *sstream->pos++ = c;
    return (unsigned char) c;
}

int
fflush(FILE *stream)
{
    int ret = 0;

    __flockfile(stream);
    if (__file_flush_wbuf(stream) == EOF)
        ret = EOF;
    if (stream->flush)
        ret = ret == EOF ? ret : stream->flush(stream);
    __funlock_return(stream, ret);
}

int
putc(int c, FILE *stream)
{
    int ret;

    __flockfile(stream);

    if ((stream->flags & __SWR) == 0) {
        stream->flags |= __SERR;
        ret = EOF;
    } else {
        if (stream->wbuf != NULL && stream->wbuf_size != 0) {
            stream->wbuf[stream->wbuf_len++] = (unsigned char) c;
            if (stream->wbuf_len == stream->wbuf_size && __file_flush_wbuf(stream) == EOF)
                ret = EOF;
            else
                ret = (unsigned char) c;
        } else if (stream->put((char) c, stream) < 0) {
            stream->flags |= __SERR;
            ret = EOF;
        } else {
            ret = (unsigned char) c;
        }
    }

    __funlockfile(stream);
    return ret;
}

int
fputc(int c, FILE *stream)
{
    return putc(c, stream);
}

int
putchar(int c)
{
    return fputc(c, stdout);
}

size_t
fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    const unsigned char *p = (const unsigned char *) ptr;
    size_t total = size * nmemb;
    size_t written = 0;

    if (size == 0 || nmemb == 0)
        return 0;

    __flockfile(stream);

    if ((stream->flags & __SWR) == 0) {
        stream->flags |= __SERR;
        __funlock_return(stream, 0);
    }

    if (__file_flush_wbuf(stream) == EOF) {
        __funlock_return(stream, 0);
    }

    if (stream->write != NULL) {
        int ret = stream->write((const char *) p, total, stream);

        if (ret < 0) {
            stream->flags |= __SERR;
            __funlock_return(stream, 0);
        }
        written = (size_t) ret;
    } else {
        while (written < total) {
            if (stream->put((char) p[written], stream) < 0) {
                stream->flags |= __SERR;
                break;
            }
            ++written;
        }
    }

    __funlockfile(stream);
    return written / size;
}

int
fputs(const char *str, FILE *stream)
{
    size_t len = strlen(str);
    return fwrite(str, 1, len, stream) == len ? 0 : EOF;
}

int
puts(const char *str)
{
    if (fputs(str, stdout) == EOF)
        return EOF;
    return putchar('\n');
}
