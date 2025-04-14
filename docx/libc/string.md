    String.h

## 函数列表

### 内存操作函数

#### memcpy

函数原型

`void  *memcpy(void *s1, const void *s2, size_t n);`

作用

将s2指向的内存区域的前n个字节复制到s1指向的内存区域.

#### mempcpy

函数原型

`void  *mempcpy(void *s1, const void *s2, size_t n);`

作用

类似于memcpy,但是返回的目标内存区域的下一个字节的地址

#### memccpy

函数原型

`void  *memccpy(void *s1, const void *s2, int count, size_t n);`

作用

复制s2指向内存区域的前n个字节到s1,知道遇到字符count

#### memmove

函数原型

`void  *memmove(void *s1, const void *s2, size_t n);`

作用

类似与memcpy,但可以处理内存区域重叠的情况

#### memchr

函数原型

`void  *memchr(const void *s, int c , size_t n);`

作用

在s指向的内存区域的前n个字节中查找字符c

#### memrchr

函数原型

`void  *memrchr(const void *s, intc, size_t n);`

作用

类似与memchr但从内存区域的末尾开始查找

#### memmem

函数原型

`void  *memmem(const void *s1, size_t n1, cons tvoid *s2, size_t n2);`

作用

在s1指向的内存区域查找s2指向的内存区域

#### memset

`void  *memset(void *s, int c, size_t n);`

作用

将s指向的内存区域的前n个字节设置为c.

#### memcmp

函数原型

`int memcmp(const void *s1, const void *s2, size_t n);`

作用

比较s1和s2指向的内存区域的前n个字节.

#### memlcp

函数原型

`size_t memlcp(const void *s1, const void *s2, size_t n);`

作用

返回s1和s2指向的内存区域的前n个字节的最长公共前缀的长度

### 字符串操作函数

#### strcpy

函数原型

`char  *strcpy(char *s1, const char *s2);`

作用

将s2指向的字符串复制到s1指向的内存区域

#### strpcpy

函数原型

`char  *strpcpy(char *s1, const char *s2);`

作用

类似于strcpy,但返回的是目标字符串的下一个字符的地址

#### strncpy

函数原型

`char  *strncpy(char *s1, const char *s2, size_t n);`

作用

将s2指向的字符串的前n个字符复制到s1指向的内存区域

#### stpncpy

函数原型

`char  *stpncpy(char *s1, const char *s2, size_t n);`

作用

将s2指向的字符串的前n个字符复制到s1，返回s1.

#### strcat

函数原型

`char  *strcat(char *s1, const char *s2);`

作用

将s2指向的字符串追加到s1指向的字符串末尾，返回s1.

#### strncat

函数原型

`char  *strncat(char *s1, const char *s2, size_t n);`

作用

将s2指向的字符串的前n个字符追加到s1指向字符串末尾。返回s1.

#### strcmp

函数原型

`int strcmp(const char *s1, const char *s2);`

作用

比较s1和s2指向的字符串。返回值为0表示相等，负值表示s1

#### strncmp

函数原型

`int strncmp(const char *s1, const char *s2, size_t n);`

作用

比较s1和s2指向的字符串的前n个字符。返回值为0相等，大于0表示s1大于s2, 小于0表示s1小于s2

#### strcasecmp

函数原型

`int strcasecmp(const char *s1, const char *s2);`

作用

类似与 `strcmp` 但是忽略大小写

#### strncasecmp

函数原型

`int strncasecmp(const char *s1, const char *s2, size_t n);`

作用

类似 `strncmp`，但是忽略大小写

#### strcoll

函数原型

`int strcoll(const char *s1, const char *s2);`

作用

根据当前区域设置比较s1和s2指向的字符串，返回值0表示想等，大于0表示s1>s2,小于0表示s1<s2

#### strchr

函数原型

`char  *strchr(const char *s, int c);`

作用

在s指向的字符串中查找字符c

#### strnchr

函数原型

`char  *strnchr(const char *s, size_t n, int c);`

作用

在s指向的字符串的前n个字符中查找字符c

#### strlcpy

函数原型

`size_t strlcpy(char *s1, const char *s2, size_t n);`

作用

将s2指向的字符串的复制到s1指向的位置，确保s1以null结尾

#### strlcat

函数原型

`size_t strlcat(char *s1, const char *s2, size_t n);`

作用

将s2指向的字符串中追加到s1指向的字符串末尾，确保s1以null结尾

#### strcspn

函数原型

`size_t strcspn(const char *s1, const char *s2);`

作用

返回s1指向的字符串中不包含s2指向的字符集的最长前缀的长度

#### strspn

函数原型

`size_t strspn(const char *s1, const char *s2);`

作用

返回s1指向字符串中不包含s2指向的字符集的最长前缀的长度

#### strpbrk

函数原型

`char  *strpbrk(const char *s1, const char *s2);`

作用

在s1指向的字符串中查找s2指向的字符集中的任意字符

#### strrchr

函数原型

`char  *strrchr(const char *s, int c);`

作用

在s指向的字符串中从末尾开始查找字符c

#### strstr

函数原型

`char  *strstr(const char *s1, const char *s2);`

作用

在s1指向的字符串中从末尾开始查找字符c

#### strnstr

函数原型

`char  *strnstr(const char *s1, const char *s2, size_t n);`

作用

在s1指向的字符串的前n个字符中查找字符c

#### strcasestr

函数原型

char  *strcasestr(cons tchar*s1, const char *s2);

作用

类似于strstr,但不区分大小写

#### strncasestr

函数原型

`char  *strncasestr(const char *s1, const char *s2, size_t n);`

作用

类似于strnstr，但不区分大小写

#### strlen

函数原型

`size_t strlen(const char *s);`

作用

返回s指向的字符串的长度

#### strnlen

函数原型

`size_t strnlen(const char *s, size_t n);`

作用

返回s指向的字符串的前n个字符的长度

#### strxfrm

函数原型

`size_t strxfrm(char *s1, const char *s2, size_t n);`

作用

根据当前区域设置转换s2指向的字符串，并将结果存储在s1中

#### strtok

函数原型

`char  *strtok(char *s1, const char *s2);`

作用

将s1指向的字符串分割位标记，使用s2指向的字符集作为分隔符

#### strtok_r

函数原型

`char  *strtok_r(char *s1, const char *s2, char **lasts);`

作用

类似strok,但支持重入

#### strdup

函数原型

`char  *strdup(constc har *s);`

作用

复制s指向的字符串，并返回新字符串的指针

#### strndup

函数原型

`char  *strndup(const char *s, size_t n);`

作用

复制s指向的字符串的前n个字符，并返回新字符串的指针

#### strerror

函数原型

`char  *strerror(int errnum);`

作用

返回与错误号errnum对应的错误信息字符串
