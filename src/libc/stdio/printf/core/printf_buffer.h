#ifndef _MICROCRT_PRINTF_BUFFER_H_
#define _MICROCRT_PRINTF_BUFFER_H_

#include <stddef.h>

#include "printf_out_core.h"

/* Writes one character into a C-string sink. */
int __printf_cstr_put(int ch, void *cookie);

/* Writes a byte span into a C-string sink. */
int __printf_cstr_write(const char *src, size_t len, void *cookie);

/* Flushes a C-string sink. */
int __printf_cstr_flush(void *cookie);

/* Appends the terminating NUL for a C-string sink. */
int __printf_cstr_finalize(void *cookie);

/* Acquires a C-string sink. */
void __printf_cstr_lock(void *cookie);

/* Releases a C-string sink. */
void __printf_cstr_unlock(void *cookie);

/* Reports whether a C-string sink accepts writes. */
int __printf_cstr_writable(void *cookie);

/* Marks a C-string sink as failed. */
void __printf_cstr_mark_error(void *cookie);

/* Binds a `struct __printf_out` to a C-string sink backend. */
void __printf_buffer_init(struct __printf_out *out, struct __printf_cstr_out *buf);

/* Runs a selected printf core against an unbounded C-string sink. */
int __printf_buffer_route(char *s, const char *fmt, va_list ap,
                          int (*core)(struct __printf_out *out, const char *fmt, va_list ap));

/* Runs a selected printf core against a bounded C-string sink. */
int __printf_buffer_route_n(char *s, size_t n, const char *fmt, va_list ap,
                            int (*core)(struct __printf_out *out, const char *fmt, va_list ap));

#endif
