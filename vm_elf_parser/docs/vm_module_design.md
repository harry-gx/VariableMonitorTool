# ELF 解析模块详细模块设计

## 接口可见性说明`r`n`r`n`include/` 目录只暴露 `vm_monitor_variables.h` 和 `vm_status.h`。下面的 ELF、DWARF、类型图、reader 和 registry 结构与函数是模块内部接口，头文件位于 `private/`，用于模块内部实现、CLI 和模块测试，不允许 UI 直接包含。`r`n`r`n## 内部数据结构

### `vm_symbol_t`

| 字段 | 含义 |
|---|---|
| `name` | 符号名称，生命周期依附 `vm_elf_t`。 |
| `address` | 符号链接地址。 |
| `size` | 符号字节数。 |
| `section_index` | 符号所在 section 索引。 |
| `section_flags` | section 属性，用于判断读写属性。 |
| `binding` | ELF symbol binding。 |

### `vm_variable_view_t`

| 字段 | 含义 |
|---|---|
| `name` | 变量名。 |
| `address` | 变量实际内存地址。 |
| `size` | 变量字节数。 |
| `section_index` | 所属 section。 |
| `binding` | 变量绑定类型。 |
| `readable` | 是否可读。 |
| `writable` | 是否可写。 |
| `volatile_hint` | 是否带 volatile 提示。 |

### `vm_type_node_view_t`

类型图的节点视图。主要字段如下：

| 字段 | 含义 |
|---|---|
| `die_offset` | 当前 DIE 在 `.debug_info` 中的偏移。 |
| `type_die_offset` | 当前节点引用的类型 DIE。 |
| `parent_die_offset` | 父 DIE 偏移，用于成员、枚举值和数组维度归属。 |
| `name` | 类型名、变量名或成员名。 |
| `byte_size` | 字节数。 |
| `kind` | 节点类型，见 `vm_type_kind_t`。 |
| `member_offset` | 成员相对聚合起始地址的字节偏移。 |
| `bit_offset` / `bit_size` | 位域偏移和位宽。 |
| `location_address` | 静态变量绝对地址。 |
| `enum_value` | 枚举成员值。 |
| `element_count` | 数组维度元素数。 |

### `vm_monitor_variable_t`

面向 UI 的最终变量描述。

| 字段 | 含义 |
|---|---|
| `name` | 展开后的变量名，例如 `g_pid.SetPoint`、`array[0]`。 |
| `type_name` | 叶子类型名，例如 `int32_t`、`float`。 |
| `address` | 实际读写地址。 |
| `size` | 读写字节数。 |
| `writable` | 目标变量是否在可写 section。 |
| `monitorable` | 是否允许加入监控。 |
| `calibratable` | 是否允许加入标定。 |
| `bit_field` | 是否为位域。 |
| `bit_offset` | 位域在存储单元内的位偏移。 |
| `bit_size` | 位域位宽。 |

## 子模块：ELF 文件解析

实现文件：`src/vm_elf.c`

### `vm_elf_parse`

| 项目 | 说明 |
|---|---|
| 原型 | `vm_status_t vm_elf_parse(const void *bytes, size_t size, vm_elf_t **out, vm_parse_error_t *error)` |
| 输入 | ELF/AXF 文件内存、文件大小、输出句柄指针、可选错误对象。 |
| 输出 | `out` 返回解析后的 `vm_elf_t`。 |
| 行为 | 校验 ELF header、section header、字符串表和符号表，成功后复制输入内容。 |
| 失败 | 返回 `VM_INVALID`、`VM_FORMAT`、`VM_UNSUPPORTED` 或 `VM_NOMEM`。 |

### `vm_elf_parse_file`

| 项目 | 说明 |
|---|---|
| 原型 | `vm_status_t vm_elf_parse_file(const char *path, vm_elf_t **out, vm_parse_error_t *error)` |
| 输入 | ELF/AXF 文件路径。 |
| 输出 | `out` 返回解析句柄。 |
| 行为 | 读取文件到内存后调用 `vm_elf_parse()`。 |
| 失败 | 文件读取失败返回 `VM_IO`；格式错误返回 `VM_FORMAT`。 |

### `vm_elf_has_debug_info`

| 项目 | 说明 |
|---|---|
| 原型 | `int vm_elf_has_debug_info(const vm_elf_t *elf)` |
| 输入 | ELF 句柄。 |
| 输出 | 非零表示存在 `.debug_info` 和相关 DWARF 段。 |
| 行为 | 只检查段存在性，不执行 DWARF 语义解析。 |

### `vm_elf_section`

| 项目 | 说明 |
|---|---|
| 原型 | `vm_status_t vm_elf_section(const vm_elf_t *elf, const char *name, vm_elf_section_view_t *out)` |
| 输入 | ELF 句柄、section 名称。 |
| 输出 | `out` 返回段数据、大小、flags、端序。 |
| 行为 | 返回借用视图，不复制段数据。 |
| 失败 | 未找到返回 `VM_NOT_FOUND`；压缩段和 NOBITS 段返回不支持或格式错误。 |

### `vm_elf_close`

| 项目 | 说明 |
|---|---|
| 原型 | `void vm_elf_close(vm_elf_t *elf)` |
| 输入 | ELF 句柄，可为空。 |
| 输出 | 无。 |
| 行为 | 释放 `vm_elf_parse()` 或 `vm_elf_parse_file()` 创建的对象。 |

### `vm_elf_symbol_count` / `vm_elf_symbol_at`

| 项目 | 说明 |
|---|---|
| 输入 | ELF 句柄和符号索引。 |
| 输出 | 符号数量或指定符号视图。 |
| 行为 | 遍历 ELF 符号表，视图生命周期依附 ELF 句柄。 |
| 失败 | 索引越界返回 `VM_NOT_FOUND`。 |

### `vm_elf_variable_count` / `vm_elf_variable_at`

| 项目 | 说明 |
|---|---|
| 输入 | ELF 句柄和变量索引。 |
| 输出 | 静态变量数量或变量视图。 |
| 行为 | 从符号表筛选可定位对象变量，并补充可读写属性。 |

### `vm_elf_find_variable` / `vm_elf_find_variable_by_address`

| 项目 | 说明 |
|---|---|
| 输入 | ELF 句柄、变量名或地址。 |
| 输出 | 匹配变量视图。 |
| 行为 | 名称查找用于 UI 搜索；地址查找用于协议读写结果反查。 |

### `vm_parser_register_builtins`

| 项目 | 说明 |
|---|---|
| 输入 | 通用注册表对象。 |
| 输出 | 注册 `arm_gcc_elf`、`keil_armcc_axf` 等内置解析器。 |
| 行为 | 注册表复制 ID 和名称，借用解析器 ops。 |

## 子模块：DWARF 原始读取

实现文件：`src/vm_dwarf.c`、`src/vm_dwarf_abbrev.c`、`src/vm_dwarf_die.c`、`src/vm_dwarf_form.c`

### `vm_dwarf_probe_unit` / `vm_dwarf_probe_unit_ex`

| 项目 | 说明 |
|---|---|
| 输入 | `.debug_info` 起始数据、大小、端序。 |
| 输出 | `vm_dwarf_unit_header_t`。 |
| 行为 | 读取一个编译单元头，支持 DWARF32/DWARF64 判别。 |

### `vm_dwarf_abbrev_at` / `vm_dwarf_abbrev_at_offset`

| 项目 | 说明 |
|---|---|
| 输入 | `.debug_abbrev` 数据、起始偏移、缩写索引。 |
| 输出 | `vm_dwarf_abbrev_view_t`。 |
| 行为 | 返回缩写 code、tag、children 标记和属性数量。 |

### `vm_dwarf_abbrev_count` / `vm_dwarf_abbrev_count_at` / `vm_dwarf_abbrev_count_checked`

| 项目 | 说明 |
|---|---|
| 输入 | `.debug_abbrev` 数据和偏移。 |
| 输出 | 缩写条目数量。 |
| 行为 | checked 版本遇到畸形表时返回错误，避免把错误表当成短表。 |

### `vm_dwarf_abbrev_attr_at`

| 项目 | 说明 |
|---|---|
| 输入 | 缩写表、缩写索引、属性索引。 |
| 输出 | `vm_dwarf_attr_form_t`，包含属性 ID、form 和 implicit const。 |

### `vm_dwarf_die_header`

| 项目 | 说明 |
|---|---|
| 输入 | `.debug_info` 数据、DIE 偏移、缩写表数据和偏移。 |
| 输出 | `vm_dwarf_die_header_t`。 |
| 行为 | 解出 DIE 的 abbrev code、tag、children 和属性起始偏移。 |

### `vm_dwarf_form_read`

| 项目 | 说明 |
|---|---|
| 输入 | 当前 CU 内缓冲区、偏移、form、form 上下文。 |
| 输出 | `vm_dwarf_value_t` 和下一偏移。 |
| 行为 | 解析整数、字符串、block、地址、引用、section offset 等 Form。 |
| 限制 | 输出中字符串和 block 借用输入缓冲区。 |

### `vm_dwarf_form_skip`

| 项目 | 说明 |
|---|---|
| 输入 | 缓冲区、偏移、form、地址宽度。 |
| 输出 | 下一偏移。 |
| 行为 | DWARF4/DWARF32/little-endian 的兼容跳过接口。 |

### `vm_dwarf_walk`

| 项目 | 说明 |
|---|---|
| 输入 | `vm_dwarf_sections_t`、DIE 回调和上下文。 |
| 输出 | 按顺序向回调发送 `vm_dwarf_die_view_t`。 |
| 行为 | 遍历所有 CU，消费 null DIE，不向上层暴露 null DIE。 |
| 生命周期 | DIE 属性数组仅在回调期间有效。 |
| 失败 | `vm_dwarf_error_t` 返回 section、offset、form 和错误信息。 |

### `vm_dwarf_die_attribute`

| 项目 | 说明 |
|---|---|
| 输入 | DIE 视图和属性 ID。 |
| 输出 | 属性值指针。 |
| 行为 | 在当前 DIE 属性数组中查找属性；找不到返回空指针。 |

### `vm_dwarf_location_address`

| 项目 | 说明 |
|---|---|
| 输入 | location 属性值、地址宽度、端序。 |
| 输出 | 绝对地址。 |
| 行为 | 当前只支持单个 `DW_OP_addr`。 |
| 限制 | location list、寄存器位置和复杂表达式返回 `VM_UNSUPPORTED`。 |

### `vm_dwarf_member_offset`

| 项目 | 说明 |
|---|---|
| 输入 | member location 属性。 |
| 输出 | 成员字节偏移。 |
| 行为 | 支持常量非负偏移和单个 `DW_OP_plus_uconst`。 |

## 子模块：类型图

实现文件：`src/vm_type_graph.c`

### `vm_type_graph_build`

| 项目 | 说明 |
|---|---|
| 输入 | DWARF section 视图。 |
| 输出 | `vm_type_graph_t`。 |
| 行为 | 遍历 DWARF DIE，复制名称，建立节点和引用关系。 |
| 原子性 | 失败时不发布半成品 graph。 |

### `vm_type_graph_destroy`

释放类型图对象，可接收空指针。

### `vm_type_graph_count` / `vm_type_graph_at`

返回所有节点数量或按索引返回节点视图。

### `vm_type_graph_find_die`

按 DIE 偏移查找节点。用于类型引用、成员归属和 UI 调试定位。

### `vm_type_graph_variable_count` / `vm_type_graph_variable_at`

遍历 DWARF 中的变量节点，包含可定位和不可定位变量。

### `vm_type_graph_static_address_count`

返回拥有绝对静态地址的变量数量。监控变量清单主要基于这类变量展开。

### `vm_type_graph_find_variable` / `vm_type_graph_find_variable_by_address` / `vm_type_graph_find_variable_containing`

| 函数 | 用途 |
|---|---|
| `find_variable` | 按变量名查找。 |
| `find_variable_by_address` | 按变量起始地址精确查找。 |
| `find_variable_containing` | 查找包含指定地址的聚合变量。 |

### `vm_type_graph_member_count` / `vm_type_graph_member_at`

按结构体或联合体 DIE 返回直接成员。不会自动递归，调用方负责递归展开。

### `vm_type_graph_find_member_containing`

按聚合 DIE 和字节偏移查找包含该偏移的成员，用于地址反查和位域定位。

### `vm_type_graph_find_enumerator` / `vm_type_graph_find_enumerator_by_value`

按枚举 DIE 查找枚举成员名称或值。

### `vm_type_graph_enumerator_count` / `vm_type_graph_enumerator_at`

遍历枚举类型的所有枚举成员。

### `vm_type_graph_resolve`

| 项目 | 说明 |
|---|---|
| 输入 | graph 和任意 DIE 偏移。 |
| 输出 | `vm_type_resolution_t`，包含解析后的类型节点和 const/volatile 修饰。 |
| 行为 | 跟随 variable、typedef、const、volatile。 |
| 停止条件 | 指针、数组、结构体、联合体、枚举和基础类型。 |
| 失败 | 循环引用返回 `VM_FORMAT`，缺失类型返回 `VM_UNSUPPORTED`。 |

### `vm_type_graph_sizeof`

解析指定 DIE 对应类型字节数。数组在缺少直接 byte size 时会尝试根据元素大小和维度推导。

### `vm_type_graph_dimension_count` / `vm_type_graph_dimension_at` / `vm_type_graph_array_count`

用于数组展开。维度按声明顺序返回，未知或动态维度会保留节点但没有 `element_count`。

## 子模块：变量清单展开

实现文件：`src/vm_monitor_variables.c`

### `vm_monitor_variables_load`

| 项目 | 说明 |
|---|---|
| 原型 | `vm_status_t vm_monitor_variables_load(const char *path, vm_monitor_variable_list_t **out, char *error, size_t error_size)` |
| 输入 | AXF/ELF 路径、错误缓冲区。 |
| 输出 | `vm_monitor_variable_list_t`。 |
| 行为 | 解析文件、构建类型图、展开变量叶子节点，并按地址和名称生成 UI 列表。 |
| 展开 | 支持结构体成员、联合体成员、数组元素和位域叶子。 |
| 权限 | 根据 section 属性设置 `writable`、`monitorable`、`calibratable`。 |

### `vm_monitor_variable_count`

返回变量列表条目数。空列表返回 0。

### `vm_monitor_variable_at`

按索引返回变量描述。索引越界返回 `VM_NOT_FOUND`。

### `vm_monitor_variable_list_destroy`

释放变量列表及其内部存储，可接收空指针。

## 子模块：值编解码

实现文件：`src/vm_value_codec.c`

### `vm_value_format`

| 项目 | 说明 |
|---|---|
| 输入 | 类型元数据、目标字节、字节数、输出文本缓冲区。 |
| 输出 | 十进制字符串。 |
| 行为 | 根据 `type_name` 和 `size` 解析整数、浮点、枚举、指针和位域。 |
| 失败 | 元数据缺失、长度不足、输出缓冲区不足或类型不支持时返回错误。 |

### `vm_value_to_double`

| 项目 | 说明 |
|---|---|
| 输入 | 类型元数据和目标字节。 |
| 输出 | `double` 数值。 |
| 用途 | 曲线绘制统一使用 double 轴值。 |
| 限制 | 聚合类型和无法数值化的类型返回不支持。 |

### `vm_value_encode`

| 项目 | 说明 |
|---|---|
| 输入 | 类型元数据、当前字节、十进制文本、输出缓冲区。 |
| 输出 | 写入字节和实际写入长度。 |
| 行为 | 把 UI 输入转成目标变量内存布局；位域只更新目标位。 |
| 失败 | 输入不是十进制数、超出目标类型范围或输出容量不足时返回错误。 |

## 子模块：读取工具

实现文件：`src/vm_reader.c`

### `vm_read_uint`

按指定端序读取 1、2、4 或 8 字节无符号整数。输入范围不足返回 `VM_FORMAT`。

### `vm_reader_range`

检查 `[offset, offset + length)` 是否位于 reader 缓冲区内。返回非零表示范围有效。

## 子模块：解析器注册表

实现文件：`src/vm_registry.c`、`src/vm_adapters.c`

### `vm_registry_create`

创建空注册表。

### `vm_registry_destroy`

销毁注册表。存在未释放引用时应返回错误或拒绝销毁，调用方应先 release。

### `vm_registry_add`

注册一个解析器适配器。注册表复制 `id` 和 `name`，借用 `ops` 指针。

### `vm_registry_remove`

按 ID 移除适配器。存在 active reference 时不应移除。

### `vm_registry_count` / `vm_registry_at`

遍历注册表元数据。

### `vm_registry_acquire` / `vm_registry_release`

按 ID 获取或释放解析器 ops。调用方需要在解析期间持有引用，避免适配器被移除。

## 函数调用约束

1. 所有输出指针参数在失败时不应被调用方当作有效结果使用。
2. 所有对象销毁函数允许传入空指针。
3. 返回的字符串指针默认借用模块对象内部存储，不能由调用方释放。
4. UI 不允许越过 `vm_monitor_variables_load()` 直接解析 DWARF 作为正式路径；DWARF 接口主要用于 CLI、测试和模块内部组合。
5. 标定写入前应先用 `vm_value_encode()` 做类型和范围校验，再交给通信模块按地址写入。

