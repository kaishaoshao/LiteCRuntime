#ifndef _MICROCRT_PRINTF_OUT_CORE_H_
#define _MICROCRT_PRINTF_OUT_CORE_H_

#include <stdio.h>
#include <stddef.h>

struct __printf_out {
    void *cookie;
    int (*put)(int ch, void *cookie);
    int (*write)(const char *buf, size_t len, void *cookie);
    int (*flush)(void *cookie);
    int (*finalize)(void *cookie);
    void (*lock)(void *cookie);
    void (*unlock)(void *cookie);
    int (*writable)(void *cookie);
    void (*mark_error)(void *cookie);
};

struct __printf_cstr_out {
    char *pos;
    char *end;
    size_t cap;
};

/* Core symbols generated from the shared printf core template. */
int __printf_core_default(struct __printf_out *out, const char *fmt, va_list ap);
int __printf_core_integer(struct __printf_out *out, const char *fmt, va_list ap);
int __printf_core_full(struct __printf_out *out, const char *fmt, va_list ap);

/* Writes a byte span to the output sink and updates the produced length. */
int __printf_out_write(struct __printf_out *out, int *stream_len, const char *buf, size_t len);

/* Locks the sink and checks that writes are permitted. */
int __printf_out_begin(struct __printf_out *out);

/* Marks the sink as failed. */
void __printf_out_fail(struct __printf_out *out);

/* Finalizes the sink and returns the final stream length. */
int __printf_out_finish(struct __printf_out *out, int stream_len);

/* Marks failure and finalizes the sink in one step. */
int __printf_out_finish_failed(struct __printf_out *out);

#endif
