#include "printf_out_core.h"

/* Writes a byte span to the output sink and updates the produced length. */
int
__printf_out_write(struct __printf_out *out, int *stream_len, const char *buf, size_t len)
{
    if (len == 0)
        return 0;

    *stream_len += (int) len;

    if (out->write != NULL)
        return out->write(buf, len, out->cookie);

    while (len--) {
        if (out->put((unsigned char) *buf++, out->cookie) < 0)
            return -1;
    }
    return 0;
}

/* Locks the sink and checks that writes are permitted. */
int
__printf_out_begin(struct __printf_out *out)
{
    out->lock(out->cookie);

    if (!out->writable(out->cookie)) {
        out->mark_error(out->cookie);
        out->unlock(out->cookie);
        return EOF;
    }
    return 0;
}

/* Marks the sink as failed. */
void
__printf_out_fail(struct __printf_out *out)
{
    out->mark_error(out->cookie);
}

/* Finalizes the sink and returns the final stream length. */
int
__printf_out_finish(struct __printf_out *out, int stream_len)
{
    if (stream_len >= 0 && out->finalize != NULL) {
        if (out->finalize(out->cookie) < 0) {
            out->mark_error(out->cookie);
            stream_len = -1;
        }
    }
    out->unlock(out->cookie);
    return stream_len;
}

/* Marks failure and finalizes the sink in one step. */
int
__printf_out_finish_failed(struct __printf_out *out)
{
    __printf_out_fail(out);
    return __printf_out_finish(out, -1);
}
