#include <stdio.h>

typedef __SIZE_TYPE__ ssize_t;
ssize_t _write(int __fd, const void *__buf, size_t __nbyte);
ssize_t _read(int __fd, void *__buf, size_t __nbyte);

#define MICROCRT_STDIO_WBUF_SIZE 64

#define FDEV_SETUP_STREAM(__cookie, __put, __get, __write, __flush, __flags, __wbuf, __wbuf_size) \
  {                                                                                                  \
      .flags = (__flags),                                                                            \
      .cookie = (__cookie),                                                                          \
      .put = (__put),                                                                                \
      .get = (__get),                                                                                \
      .write = (__write),                                                                            \
      .flush = (__flush),                                                                            \
      .close = NULL,                                                                                 \
      .wbuf = (__wbuf),                                                                              \
      .wbuf_size = (__wbuf_size),                                                                    \
      .wbuf_len = 0,                                                                                 \
  }

static int
mculib_getc(FILE *file)
{
    (void) file;
    char c = 0;

    _read(0, &c, 1);
    return c;
}

static int
mculib_write(const char *buf, size_t len, FILE *file)
{
    int fd = (int) (intptr_t) file->cookie;

    return (int) _write(fd, buf, len);
}

static int
mculib_stdout_putc(char c, FILE *file)
{
    return mculib_write(&c, 1, file) < 0 ? EOF : (int) c;
}

static int
mculib_stderr_putc(char c, FILE *file)
{
    return mculib_write(&c, 1, file) < 0 ? EOF : (int) c;
}

static int
mculib_flush(FILE *file)
{
    (void) file;
    return 0;
}

static unsigned char __stdout_wbuf[MICROCRT_STDIO_WBUF_SIZE];
static unsigned char __stderr_wbuf[MICROCRT_STDIO_WBUF_SIZE];

static FILE __stdin =
    FDEV_SETUP_STREAM((void *)(intptr_t) 0, NULL, mculib_getc, NULL, NULL, __SRD, NULL, 0);
static FILE __stdout =
    FDEV_SETUP_STREAM((void *)(intptr_t) 1, mculib_stdout_putc, NULL, mculib_write, mculib_flush,
                      __SWR, __stdout_wbuf, sizeof(__stdout_wbuf));
static FILE __stderr =
    FDEV_SETUP_STREAM((void *)(intptr_t) 2, mculib_stderr_putc, NULL, mculib_write, mculib_flush,
                      __SWR, __stderr_wbuf, sizeof(__stderr_wbuf));

FILE *const stdin = &__stdin;
FILE *const stdout = &__stdout;
FILE *const stderr = &__stderr;
