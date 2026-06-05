#ifndef _MICROCRT_PRINTF_FILE_H_
#define _MICROCRT_PRINTF_FILE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "printf_out_core.h"

#ifndef MICROCRT_PRINTF_ENABLE_AUTO_FLUSH
#define MICROCRT_PRINTF_ENABLE_AUTO_FLUSH 0
#endif

struct __file_str {
    struct __file file;
    char         *pos;
    char         *end;
    size_t        size;
    bool          alloc;
};

int __file_str_get(FILE *stream);
int __file_str_put(char c, FILE *stream);
int __file_str_put_alloc(char c, FILE *stream);

#define FDEV_STRING_WRITE_END(_s, _n) (((int)(_n) < 0) ? NULL : ((_n) ? (_s) + (_n) - 1 : (_s)))

#define FDEV_SETUP_STRING_WRITE(_s, _end)                                    \
    {                                                                        \
        .file = { .flags = __SWR, .put = __file_str_put, .write = NULL,      \
                  .flush = NULL, .close = NULL, .wbuf = NULL, .wbuf_size = 0,\
                  .wbuf_len = 0, __LOCK_INIT_NONE },                         \
        .pos = (_s),                                                         \
        .end = (_end),                                                       \
    }

#define FDEV_SETUP_STRING_ALLOC()                                                  \
    {                                                                              \
        .file = { .flags = __SWR, .put = __file_str_put_alloc, .write = NULL,      \
                  .flush = NULL, .close = NULL, .wbuf = NULL, .wbuf_size = 0,      \
                  .wbuf_len = 0, __LOCK_INIT_NONE },                               \
        .pos = NULL,                                                               \
        .end = NULL,                                                               \
        .size = 0,                                                                 \
        .alloc = false,                                                            \
    }

#define FDEV_SETUP_STRING_ALLOC_BUF(_buf, _size)                                   \
    {                                                                              \
        .file = { .flags = __SWR, .put = __file_str_put_alloc, .write = NULL,      \
                  .flush = NULL, .close = NULL, .wbuf = NULL, .wbuf_size = 0,      \
                  .wbuf_len = 0, __LOCK_INIT_NONE },                               \
        .pos = _buf,                                                               \
        .end = (char *)(_buf) + (_size),                                           \
        .size = _size,                                                             \
        .alloc = false,                                                            \
    }

#ifdef __STDIO_LOCKING
void __flockfile_init(FILE *f);
#define __LOCK_NONE      ((_LOCK_RECURSIVE_T)(uintptr_t)1)
#define __LOCK_INIT_NONE .lock = __LOCK_NONE
#else
#define __LOCK_INIT_NONE
#endif

#define __funlock_return(f, v) \
    do {                       \
        __funlockfile(f);      \
        return (v);            \
    } while (0)

/* Acquires the stdio lock used by MicroCRT FILE streams. */
void __flockfile(FILE *f);

/* Releases the stdio lock used by MicroCRT FILE streams. */
void __funlockfile(FILE *f);

/* Writes one character into a FILE sink. */
int __printf_file_put(int ch, void *cookie);

/* Writes a byte span into a FILE sink. */
int __printf_file_write(const char *buf, size_t len, void *cookie);

/* Flushes a FILE sink when the backend supports it. */
int __printf_file_flush(void *cookie);

/* Finalizes a FILE sink after formatting completes. */
int __printf_file_finalize(void *cookie);

/* Acquires a FILE sink lock. */
void __printf_file_lock(void *cookie);

/* Releases a FILE sink lock. */
void __printf_file_unlock(void *cookie);

/* Reports whether the FILE sink is writable. */
int __printf_file_writable(void *cookie);

/* Marks a FILE sink as failed. */
void __printf_file_mark_error(void *cookie);

/* Binds a `struct __printf_out` to a FILE sink backend. */
void __printf_file_init(struct __printf_out *out, FILE *stream);

/* Runs a selected printf core against a FILE sink. */
int __printf_file_route(FILE *stream, const char *fmt, va_list ap,
                        int (*core)(struct __printf_out *out, const char *fmt, va_list ap));

#endif
