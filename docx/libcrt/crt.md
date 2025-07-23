
# libcrt

## softfloat函数分类

### 1. 整数运算函数

* 绝对值函数: absvdi2.c, absvsi2.c, absvti2.c
* 加法函数: addvdi3.c, addvsi3.c, addvti3.c
* 减法函数: subvdi3.c, subvsi3.c, subvti3.c
* 乘法函数: mulvdi3.c, mulvsi3.c, mulvti3.c
* 除法函数: divdi3.c, divsi3.c, divti3.c
* 取模函数: moddi3.c, modsi3.c, modti3.c

### 2. 浮点数运算函数

* 加法: adddf3.c, addsf3.c, addtf3.c
* 减法: subdf3.c, subsf3.c, subtf3.c
* 乘法: muldf3.c, mulsf3.c, multf3.c
* 除法: divdf3.c, divsf3.c, divtf3.c

### 3. 位运算函数

* 移位: ashldi3.c, ashlti3.c, ashrdi3.c, ashrti3.c, lshrdi3.c, lshrti3.c
* 字节交换: bswapdi2.c, bswapsi2.c
* 计数: clzdi2.c, clzsi2.c, clzti2.c, ctzdi2.c, ctzsi2.c, ctzti2.c
* 查找: ffsdi2.c, ffssi2.c, ffsti2.c
* 奇偶性: paritydi2.c, paritysi2.c, parityti2.c
* 位计数: popcountdi2.c, popcountsi2.c, popcountti2.c

### 4. 比较函数

* 整数比较: cmpdi2.c, cmpti2.c, ucmpdi2.c, ucmpti2.c
* 浮点数比较: comparedf2.c, comparesf2.c, comparetf2.c

### 5. 类型转换函数

* 浮点数扩展: extendsfdf2.c, extendhfsf2.c, extenddftf2.c, extendhftf2.c, extendsftf2.c
* 浮点数截断: truncdfhf2.c, truncdfsf2.c, truncsfhf2.c, trunctfdf2.c, trunctfhf2.c, trunctfsf2.c
* 浮点数到整数: fixdfdi.c, fixsfdi.c, fixdfsi.c, fixdfti.c, fixsfsi.c, fixsfti.c, fixtfdi.c, fixtfsi.c, fixtfti.c
* 整数到浮点数: floatdidf.c, floatsidf.c, floatsisf.c, floattidf.c, floattisf.c

### 6. 复数运算函数

* 复数除法: divdc3.c, divsc3.c, divtc3.c
* 复数乘法: muldc3.c, mulsc3.c, multc3.c

### 7. 特殊函数

* 缓存清除: clear_cache.c
* 操作系统版本检查: os_version_check.c
* 跳板设置: trampoline_setup.c
* 整数工具: int_util.c
