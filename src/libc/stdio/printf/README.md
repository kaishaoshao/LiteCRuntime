# `MicroCRT` `printf` 结构说明

本文档只回答三件事：

* 代码现在按什么结构组织
* 主链怎么走
* 后续维护时必须遵守什么规则

## 目录

`src/libc/stdio/`

* wrapper:
  `printf.c` `vprintf.c` `fprintf.c` `vfprintf.c`
  `sprintf.c` `snprintf.c`
  `iprintf.c`
  `printf_full.c`
* backend bridge:
  `fileops.c` `iob.c`

`src/libc/stdio/printf/`

* `core/`
  `printf_core_private.h`
  `printf_out_core.h`
  `printf_file.h`
  `printf_buffer.h`
  `printf_float_support.h`
* `entry/`
  `printf_core_default.c`
  `printf_core_integer.c`
  `printf_core_full.c`
* `parse/`
  `_printf_parser.c`
  `_printf_positional.c`
  `_printf_conversion_dispatch.c`
* `text/`
  `_printf_text_core.c`
  `_printf_c.c`
  `_printf_s.c`
  `_printf_n.c`
* `int/`
  `_printf_int_dec.c`
  `_printf_int_base.c`
  `_printf_d.c`
  `_printf_u.c`
  `_printf_o.c`
  `_printf_x.c`
  `_printf_b.c`
* `float/`
  `printf_float.h`
  `_printf_fp_core.c`
  `_printf_f.c`
  `_printf_e.c`
  `_printf_g.c`
  `_printf_a.c`
* `support/`
  `ultoa_invert.c`
  `dtoa_engine.c`
  `dtox_engine.c`
  `ldtoa_engine.c`
  `ldtox_engine.c`
* float engine binding:
  `dtoa.h`

## 主链

这里的主链不是 wrapper 展开图，而是实际代码结构图。下面这张图只保留四层：

* core
* shared dispatcher
* file backend
* buffer backend

```text
+--------------------------------------------------------------------------------------+
|                    [Core: __printf_core_default / integer / full]                    |
| 文件: entry/printf_core_default.c                                                    |
|       entry/printf_core_integer.c                                                    |
|       entry/printf_core_full.c                                                       |
| 模板: core/printf_core_template.inc                                                  |
| 负责函数: __printf_parse_scan_literal                                                |
|           __printf_parse_conversion_spec                                             |
|           __printf_dispatch_conversion                                               |
|           __printf_out_finish                                                        |
+--------------------------------------------------------------------------------------+
                                           |
                                           v
+--------------------------------------------------------------------------------------+
|                     [Shared Dispatcher: parse/_printf_conversion_dispatch.c]          |
| 统一分发函数: __printf_dispatch_conversion                                            |
| text 路径: __printf_text_char_entry / __printf_text_string_entry                     |
|            __printf_text_percent_n_checked                                           |
| int  路径: __printf_int_dec_entry / __printf_int_udec_entry /                        |
|            __printf_int_oct_entry / __printf_int_hex_entry /                         |
|            __printf_int_bin_entry / __printf_int_pointer_entry                       |
| float路径: __printf_float_f / __printf_float_e / __printf_float_g / __printf_float_a|
| helper:     __printf_text_char / __printf_text_string /                              |
|             __printf_int_format_dec / __printf_int_format_base /                     |
|             __printf_int_format_pointer /                                            |
|             __printf_float_format_dec / __printf_float_format_hex                    |
+--------------------------------------------------------------------------------------+
                                           |
                      +--------------------+--------------------+
                      | (同一 dispatcher，把结果写到不同 sink)  |
                      v                                         v
+------------------------------------------------+   +------------------------------------------------+
| [Output A: File Backend Interface]             |   | [Output B: Buffer Backend Interface]          |
| 文件: stdio/fileops.c                          |   | 文件: core/printf_buffer.c                    |
|      core/printf_file.h                        |   |      core/printf_buffer.h                     |
| 公共 route: __printf_file_route                |   | 公共 route: __printf_buffer_route /              |
|                                                |   |              __printf_buffer_route_n             |
| 入口: vfprintf / vfiprintf / vfprintf_full     |   |      __printf_buffer_init                     |
|      -> __printf_file_route(..., core)         |   | 写接口: __printf_cstr_put / __printf_cstr_write |
| 写接口: __printf_file_put / __printf_file_write|   | 收尾:   __printf_cstr_finalize               |
| 收尾:   __printf_file_flush / finalize         |   |                                                |
+------------------------------------------------+   +------------------------------------------------+
```

对应到公开入口时，主链是：

```text
printf / fprintf / iprintf / printf_full
-> vfprintf / vfiprintf / vfprintf_full
-> __printf_file_route
-> __printf_core_default / __printf_core_integer / __printf_core_full
-> __printf_dispatch_conversion
-> file backend

sprintf / snprintf / siprintf / sprintf_full / snprintf_full
-> __printf_buffer_route / __printf_buffer_route_n
-> __printf_buffer_init
-> __printf_core_default / __printf_core_integer / __printf_core_full
-> __printf_dispatch_conversion
-> buffer backend
```

## 完整调用图

下面这张图从公开 `printf` 入口一直展开到最终 backend。

`FILE` 路线：

```text
printf
-> vfprintf                                   [src/libc/stdio/vfprintf.c]
-> __printf_file_route                        [src/libc/stdio/fileops.c]
-> __printf_core_default                      [src/libc/stdio/printf/entry/printf_core_default.c]
-> __printf_parse_scan_literal                [src/libc/stdio/printf/parse/_printf_parser.c]
-> __printf_parse_conversion_spec             [src/libc/stdio/printf/parse/_printf_parser.c]
-> __printf_dispatch_conversion               [src/libc/stdio/printf/parse/_printf_conversion_dispatch.c]
   -> __printf_text_char_entry                [src/libc/stdio/printf/text/_printf_c.c]
      -> __printf_text_char                   [src/libc/stdio/printf/text/_printf_text_core.c]
      -> __printf_text_emit_common            [src/libc/stdio/printf/text/_printf_text_core.c]
   -> __printf_text_string_entry              [src/libc/stdio/printf/text/_printf_s.c]
      -> __printf_text_string                 [src/libc/stdio/printf/text/_printf_text_core.c]
      -> __printf_text_emit_common            [src/libc/stdio/printf/text/_printf_text_core.c]
   -> __printf_int_dec_entry                  [src/libc/stdio/printf/int/_printf_d.c]
      -> __printf_int_format_dec              [src/libc/stdio/printf/int/_printf_int_dec.c]
   -> __printf_int_udec_entry                 [src/libc/stdio/printf/int/_printf_u.c]
      -> __printf_int_format_base             [src/libc/stdio/printf/int/_printf_int_base.c]
   -> __printf_int_oct_entry                  [src/libc/stdio/printf/int/_printf_o.c]
      -> __printf_int_format_base             [src/libc/stdio/printf/int/_printf_int_base.c]
   -> __printf_int_hex_entry                  [src/libc/stdio/printf/int/_printf_x.c]
      -> __printf_int_format_base             [src/libc/stdio/printf/int/_printf_int_base.c]
   -> __printf_int_pointer_entry              [src/libc/stdio/printf/int/_printf_int_base.c]
      -> __printf_int_format_pointer          [src/libc/stdio/printf/int/_printf_int_base.c]
      -> __printf_int_format_base             [src/libc/stdio/printf/int/_printf_int_base.c]
   -> __printf_float_f/e/g/a                  [src/libc/stdio/printf/float/_printf_*.c]
      -> __printf_float_format_dec/hex        [src/libc/stdio/printf/float/_printf_fp_core.c]
      -> __dtoa_engine / __dtox_engine /
         __ldtoa_engine / __ldtox_engine      [src/libc/stdio/printf/support/*.c]
-> __printf_out_finish                        [src/libc/stdio/printf/core/printf_out_core.c]
-> __printf_file_write / __printf_file_put    [src/libc/stdio/fileops.c]
-> FILE.write / FILE.put                      [include/libc/stdio.h + src/libc/stdio/iob.c]
-> backend device
```

`integer FILE` 路线：

```text
iprintf
-> viprintf                                   [src/libc/stdio/iprintf.c]
-> vfiprintf                                  [src/libc/stdio/iprintf.c]
-> __printf_file_route                        [src/libc/stdio/fileops.c]
-> __printf_core_integer                      [src/libc/stdio/printf/entry/printf_core_integer.c]
-> __printf_parse_scan_literal
-> __printf_parse_conversion_spec
-> __printf_dispatch_conversion
-> text / int only
-> __printf_out_finish
-> __printf_file_write / __printf_file_put
-> FILE.write / FILE.put
-> backend device
```

`full FILE` 路线：

```text
printf_full
-> vprintf_full                               [src/libc/stdio/printf_full.c]
-> vfprintf_full                              [src/libc/stdio/printf_full.c]
-> __printf_file_route                        [src/libc/stdio/fileops.c]
-> __printf_core_full                         [src/libc/stdio/printf/entry/printf_core_full.c]
-> __printf_parse_scan_literal
-> __printf_parse_conversion_spec
-> __printf_dispatch_conversion
-> text / int / float
-> __printf_out_finish
-> __printf_file_write / __printf_file_put
-> FILE.write / FILE.put
-> backend device
```

`buffer` 路线：

```text
snprintf
-> vsnprintf                                  [src/libc/stdio/snprintf.c]
-> __printf_buffer_route_n                    [src/libc/stdio/printf/core/printf_buffer.c]
-> __printf_buffer_init                       [src/libc/stdio/printf/core/printf_buffer.c]
-> __printf_core_default                      [src/libc/stdio/printf/entry/printf_core_default.c]
-> __printf_parse_scan_literal                [src/libc/stdio/printf/parse/_printf_parser.c]
-> __printf_parse_conversion_spec             [src/libc/stdio/printf/parse/_printf_parser.c]
-> __printf_dispatch_conversion               [src/libc/stdio/printf/parse/_printf_conversion_dispatch.c]
   -> text / int / float helpers             [src/libc/stdio/printf/text|int|float]
-> __printf_out_finish                        [src/libc/stdio/printf/core/printf_out_core.c]
-> __printf_cstr_write / __printf_cstr_put    [src/libc/stdio/printf/core/printf_buffer.c]
-> __printf_cstr_finalize                     [src/libc/stdio/printf/core/printf_buffer.c]
-> char* buffer
```

## family 调用图

`printf_core_*` 内部主线：

```text
__printf_core_*
-> __printf_parse_scan_literal
-> __printf_parse_conversion_spec
-> __printf_dispatch_conversion
-> __printf_out_finish
```

`text family`：

```text
__printf_dispatch_conversion
-> __printf_text_char_entry
   -> __printf_text_char
   -> __printf_text_emit_common
   -> backend

-> __printf_text_string_entry
   -> __printf_text_string
   -> __printf_text_emit_common
   -> backend

-> __printf_text_percent_n_checked
   -> __printf_text_percent_n
```

`int family`：

```text
__printf_dispatch_conversion
-> __printf_int_dec_entry
   -> __printf_int_format_dec
   -> backend

-> __printf_int_udec_entry
   -> __printf_int_format_base
   -> backend

-> __printf_int_oct_entry
   -> __printf_int_format_base
   -> backend

-> __printf_int_hex_entry
   -> __printf_int_format_base
   -> backend

-> __printf_int_bin_entry
   -> __printf_int_format_base
   -> backend

-> __printf_int_pointer_entry
   -> __printf_int_format_pointer
   -> __printf_int_format_base
   -> backend
```

`float family`：

```text
__printf_dispatch_conversion
-> __printf_dispatch_float_conversion
-> __printf_float_f / __printf_float_e / __printf_float_g
   -> __printf_float_format_dec
   -> __dtoa_engine / __ldtoa_engine
   -> backend

-> __printf_float_a
   -> __printf_float_format_hex
   -> __dtox_engine / __ldtox_engine
   -> backend
```

## core 结构

`printf_core_default.c`、`printf_core_integer.c` 和 `printf_core_full.c` 不是三份独立维护的主循环。
它们只定义 profile 宏，然后共同实例化 `core/printf_core_template.inc`。

三个 core 共同负责：

* output begin / finish
* literal scan
* parse 一个 conversion spec
* dispatch 到对应 family
* 统一错误和收尾路径

## profile 能力边界

当前三套 core 的能力边界是：

* `default`
  常规 `text + int + double`
* `default` 不支持：
  positional、宽字符、`%a`、`long double`、128 位算术
* `integer`
  `text + int`
* `integer` 不支持：
  浮点格式化
* `integer`
  对不支持的浮点 conversion 保留历史 `*float*` 占位输出
* `full`
  打开扩展能力：
  positional、宽字符、`%a`、`long double`
* `full`
  只在 `long double > 64-bit` 的后端上允许 128 位算术支撑

## family 分工

`parse/`

* `_printf_parser.c`
  literal scan、flag/width/precision/size/positional 解析
* `_printf_positional.c`
  positional `va_list` 生命周期和 seek/rewind
* `_printf_conversion_dispatch.c`
  按 conversion 字符分流到 text/int/float

`text/`

* `_printf_text_core.c`
  `%c/%s/%n` 共享输出 helper
* `_printf_c.c` `_printf_s.c` `_printf_n.c`
  薄 specifier entry

`int/`

* `_printf_int_dec.c`
  有符号十进制格式化
* `_printf_int_base.c`
  无符号进制格式化和 `%p`
* `_printf_d.c` `_printf_u.c` `_printf_o.c` `_printf_x.c` `_printf_b.c`
  薄 specifier entry

`float/`

* `_printf_fp_core.c`
  `%f/%e/%g/%a` 共用输出逻辑
* `_printf_f.c` `_printf_e.c` `_printf_g.c` `_printf_a.c`
  薄 specifier entry
* `dtoa.h`
  float profile 到 dtoa/dtox/ldtoa/ldtox 的绑定层
* `support/*.c`
  真正数值引擎

## backend 分工

`printf_out_*` 的职责固定为：

* `printf_out_core.*`
  sink 抽象和统一 begin/write/fail/finish
* `printf_file.*`
  `FILE` sink
* `printf_buffer.*`
  `char*` sink

当前 `FILE` 路线仍然是保守模型：

* `printf_out.write`
  对 `FILE` 仍然可能退化成 `put` 循环
* `printf_out`
  的能力已经强于当前 `FILE`

## 命名规则

内部函数命名必须遵守下面这组规则：

* `dispatch_*`
  只用于 conversion router
* `entry_*`
  只用于单个 specifier 入口
* `format_*`
  只用于 family 内部格式化算法
* `parse_scan_*`
  只用于 literal / token 扫描
* `parse_accept_*`
  只用于 parser 状态推进
* `parse_finalize_*`
  只用于 parser 收尾归一化
* `positional_init/seek/rewind/cleanup`
  只用于 positional 生命周期
* `out_*`
  只用于 sink/backend

如果一个新函数名不属于这些类，通常说明职责边界还没想清楚。

## 维护规则

后续修改这套 `printf` 时，默认遵守这些规则：

* 先判断逻辑属于 `core/parse/text/int/float/out/support` 哪一层，再决定放哪里。
* 一个文件只保留一个主职责，不按“文件长短”拆分。
* 薄转发层如果没有独立语义，就合并掉。
* 新增 specifier 时，优先增加 `entry_*`，不要直接把逻辑塞进 dispatcher。
* 新增 family 算法时，优先放进 `format_*` / `*_core.c`，不要把算法塞进 entry。
* parser 改动优先收进 `_printf_parser.c` 或 `_printf_positional.c`，不要把解析流程重新展开回 core 主循环。
* backend 改动优先收进 `printf_out_*`，不要让 family helper 直接感知 `FILE` 细节。

## 当前结论

这套实现采用的是：

```text
entry wrapper
  -> backend adapter
  -> core-centered dispatch
  -> family helper graph
  -> support engines
```

这不是单文件 `vfprintf` 巨石，也不是“每个格式符一个完全独立模块”的组织方式。

当前推荐的维护方向是：

* 保持 `printf_core` 为唯一中心调度点
* 保持 `parse/text/int/float/out/support` 六层边界稳定
* 继续减少无独立语义的薄文件
* 继续把重复逻辑收回 family core 或 support core
