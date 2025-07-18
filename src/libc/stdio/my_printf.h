// my_printf.h
#ifndef MY_PRINTF_H
#define MY_PRINTF_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

// --- 功能配置宏 ---
#define MY_PRINTF_SUPPORT_LONG_LONG 1
#define MY_PRINTF_SUPPORT_FLOAT 1
#define MY_PRINTF_SUPPORT_THOUSANDS_SEPARATOR 1
#define MY_PRINTF_SUPPORT_WIDE_CHAR 1
#define MY_PRINTF_SUPPORT_DYN_ALLOC 1
#define MY_PRINTF_SUPPORT_C99_LMODS 1
#define MY_PRINTF_SUPPORT_WIDTH_PRECISION 1
#define MY_PRINTF_SUPPORT_FLAGS 1
#define MY_PRINTF_SUPPORT_PERCENT_N 1

// --- *新增* 自定义格式说明符 API ---

// 前向声明上下文结构体，避免在公共头文件中暴露过多细节
struct my_printf_ctx;

/**
 * @brief 自定义格式处理函数的指针类型
 *
 * @param ctx           指向当前打印操作的上下文
 * @param ap            指向 va_list 的指针，处理函数需要从中提取参数
 * @param flags         解析到的标志位 (如 FLAG_LEFT_JUSTIFY)
 * @param width         解析到的宽度
 * @param precision     解析到的精度
 * @return int          应返回 0
 */
typedef int (*my_printf_custom_handler_t)(struct my_printf_ctx *ctx,
                                          va_list *ap, int flags, int width,
                                          int precision);

/**
 * @brief 注册一个自定义的格式说明符处理函数
 *
 * @param specifier     您要定义的格式字符 (例如 'B', 'I', 'T')
 * @param handler       指向处理该格式的函数指针
 */
void my_printf_register_handler(char specifier,
                                my_printf_custom_handler_t handler);

// --- 公共 API 函数声明 ---
int my_printf(const char *format, ...);
int my_fprintf(FILE *stream, const char *format, ...);
int my_snprintf(char *buffer, size_t count, const char *format, ...);
int my_vfprintf(FILE *stream, const char *format, va_list arg);
int my_vsnprintf(char *buffer, size_t count, const char *format, va_list arg);

#if MY_PRINTF_SUPPORT_DYN_ALLOC
int my_asprintf(char **strp, const char *format, ...);
int my_vasprintf(char **strp, const char *format, va_list arg);
#endif

#endif // MY_PRINTF_H