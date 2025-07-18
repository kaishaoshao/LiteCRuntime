// test_main.c
#define _GNU_SOURCE // 为了在 stdio.h 中启用 asprintf
#include "my_printf.h"
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h> // 包含 free()
#include <string.h>

// --- 测试框架 ---
static int tests_passed = 0;
static int tests_failed = 0;

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"

// 宏: 已修正，使用 ##__VA_ARGS__ 来处理无参数的情况
#define TEST_CASE(name, format, ...)                                           \
  do {                                                                         \
    char my_buf[512] = {0};                                                    \
    char std_buf[512] = {0};                                                   \
    int my_ret = my_snprintf(my_buf, sizeof(my_buf), format, ##__VA_ARGS__);   \
    int std_ret = snprintf(std_buf, sizeof(std_buf), format, ##__VA_ARGS__);   \
    if (strcmp(my_buf, std_buf) == 0 && my_ret == std_ret) {                   \
      printf("[%sPASS%s] %s\n", KGRN, KNRM, name);                             \
      tests_passed++;                                                          \
    } else {                                                                   \
      printf("[%sFAIL%s] %s\n", KRED, KNRM, name);                             \
      printf("       Format:   \"%s\"\n", format);                             \
      printf("       Expected: \"%s\" (ret %d)\n", std_buf, std_ret);          \
      printf("       Actual:   \"%s\" (ret %d)\n", my_buf, my_ret);            \
      tests_failed++;                                                          \
    }                                                                          \
  } while (0)

// 自定义功能测试宏 (与硬编码的期望值比较)
#define TEST_CASE_CUSTOM(name, expected_str, expected_ret, format, ...)        \
  do {                                                                         \
    char my_buf[512] = {0};                                                    \
    int my_ret = my_snprintf(my_buf, sizeof(my_buf), format, ##__VA_ARGS__);   \
    if (strcmp(my_buf, expected_str) == 0 && my_ret == expected_ret) {         \
      printf("[%sPASS%s] %s\n", KGRN, KNRM, name);                             \
      tests_passed++;                                                          \
    } else {                                                                   \
      printf("[%sFAIL%s] %s\n", KRED, KNRM, name);                             \
      printf("       Format:   \"%s\"\n", format);                             \
      printf("       Expected: \"%s\" (ret %d)\n", expected_str,               \
             expected_ret);                                                    \
      printf("       Actual:   \"%s\" (ret %d)\n", my_buf, my_ret);            \
      tests_failed++;                                                          \
    }                                                                          \
  } while (0)

// --- 测试分类 ---
void test_integers() {
  printf("\n--- Testing Integers ---\n");
  TEST_CASE("Positive", "%d", 123);
  TEST_CASE("Negative", "%d", -123);
  TEST_CASE("Zero", "%d", 0);
  TEST_CASE("Width", "[%10d]", 123);
  TEST_CASE("Zero Pad", "[%010d]", 123);
  TEST_CASE("Left Justify", "[%-10d]", 123);
  TEST_CASE("Sign Flags", "[%+d] [% d]", 123, 123);
  TEST_CASE("Precision", "[%.8d]", 1234);
  TEST_CASE("Width & Precision", "[%10.8d]", 1234);
  TEST_CASE("0 Flag Ignored", "[%010.8d]", 1234);
  TEST_CASE("Val 0 Prec 0", "[%.0d]", 0);
  TEST_CASE("Hex", "0x%x %#X", 255, 255);
  TEST_CASE("Octal", "0%o %#o", 63, 63);
  TEST_CASE("Long Long", "%lld", 123456789012345LL);
#if MY_PRINTF_SUPPORT_THOUSANDS_SEPARATOR
  TEST_CASE_CUSTOM("Thousands Separator", "12,345,678", 10, "%'d", 12345678);
#endif
}

void test_floats() {
  printf("\n--- Testing Floats ---\n");
#if MY_PRINTF_SUPPORT_FLOAT
  TEST_CASE("Float", "%f", 3.14159);
  TEST_CASE("Float Precision", "%.3f", 3.14159);
  TEST_CASE("Float Align", "[%20.3f]", -3.14159);
  TEST_CASE("Scientific", "%e", 12345.678);
  TEST_CASE("General (f-style)", "%g", 123.45);
  TEST_CASE("General (e-style)", "%g", 1234567.8);
  TEST_CASE("General No Trail Zero", "%.10g", 123.45);
  TEST_CASE("General Alternative", "%#.10g", 123.45);
  TEST_CASE("Infinity", "%f", 1.0 / 0.0);
#if MY_PRINTF_SUPPORT_ENGINEERING_NOTATION
  TEST_CASE_CUSTOM("Engineering", "12.35e+03", 9, "%.2^f", 12345.67);
  TEST_CASE_CUSTOM("Engineering Small", "123.45e-06", 11, "%.2^f", 0.00012345);
#endif
#endif
}

void test_strings_chars() {
  printf("\n--- Testing Strings and Chars ---\n");
  TEST_CASE("String", "%s", "hello");
  TEST_CASE("Char", "%c", 'Z');
  TEST_CASE("Pointer", "%p", (void *)0x12345678);
  TEST_CASE("Percent", "%%"); // 现在这个可以正常工作了
#if MY_PRINTF_SUPPORT_WIDE_CHAR
  setlocale(LC_ALL, "");
  TEST_CASE("Wide Char", "%lc", L'Ω');
  TEST_CASE("Wide String", "%ls", L"你好");
  TEST_CASE("Wide String Precision", "[%.4ls]", L"你好世界");
#endif
}

void test_n_specifier() {
  printf("\n--- Testing %%n ---\n");
#if MY_PRINTF_SUPPORT_PERCENT_N
  int my_n = 0, std_n = 0;
  my_snprintf(NULL, 0, "12345%n", &my_n);
  snprintf(NULL, 0, "12345%n", &std_n);
  if (my_n == std_n) {
    printf("[%sPASS%s] %%n value\n", KGRN, KNRM);
    tests_passed++;
  } else {
    printf("[%sFAIL%s] %%n value (exp %d, got %d)\n", KRED, KNRM, std_n, my_n);
    tests_failed++;
  }
#endif
}

void test_dyn_alloc() {
  printf("\n--- Testing Dynamic Allocation ---\n");
#if MY_PRINTF_SUPPORT_DYN_ALLOC
  char *my_str = NULL, *std_str = NULL;
  int my_ret = my_asprintf(&my_str, "Test: %d, %.2f", 42, 1.23);
  int std_ret = asprintf(&std_str, "Test: %d, %.2f", 42, 1.23);
  if (my_ret == std_ret && strcmp(my_str, std_str) == 0) {
    printf("[%sPASS%s] asprintf\n", KGRN, KNRM);
    tests_passed++;
  } else {
    printf("[%sFAIL%s] asprintf\n", KRED, KNRM);
    tests_failed++;
  }
  free(my_str);
  free(std_str);
#endif
}

int main() {
  test_integers();
  test_floats();
  test_strings_chars();
  test_n_specifier();
  test_dyn_alloc();

  printf("\n--- Test Summary ---\n");
  printf("Total tests: %d\n", tests_passed + tests_failed);
  printf("%sPassed: %d%s\n", KGRN, tests_passed, KNRM);
  if (tests_failed > 0) {
    // 已修正参数顺序
    printf("%sFailed: %d%s\n", KRED, tests_failed, KNRM);
    return 1;
  }
  return 0;
}