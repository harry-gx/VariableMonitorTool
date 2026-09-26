# ELF 解析模块使用说明

## 运行前依赖

- C 编译器：支持 C11 的 `gcc` 或兼容编译器。
- 构建工具：GNU `make`。Windows 原生命令行、Git Bash/MSYS/MinGW 和 Linux 都可以直接执行 `make`；Makefile 会按当前 shell 自动选择目录创建和清理命令。
- 输入文件：带符号表和 DWARF 调试信息的 ARM ELF/AXF 文件。Keil 工程需要保留调试信息输出，不能只给 stripped 文件。
- 当前模块不依赖 Qt，不依赖串口/CAN 驱动，也不依赖 MCU 在线连接。

## 构建

在工程根目录执行：

```bash
make -C vm_elf_parser all
```

或者在 `vm_elf_parser` 目录内执行：

```bash
make all
```

构建输出：

| 输出 | 路径 | 用途 |
|---|---|---|
| 静态库 | `build/vm_elf_parser_make/libvm_elf_parser.a` | 供 UI 或其他可执行文件链接。 |
| 命令行工具 | `build/vm_elf_parser_make/vm_elf_parser_cli`（Windows 下为 `.exe`） | 离线输出 ELF/AXF 可监控、可标定变量清单。 |
| 中间文件 | `build/vm_elf_parser_make/obj/` | 编译产生的对象文件。 |

清理：

```bash
make -C vm_elf_parser clean
```

## 测试

执行：

```bash
make -C vm_elf_parser test
```

测试内容：

- DWARF 编译单元、缩写表、DIE 和 Form 解析。
- 值格式化和值编码。
- 类型图构建、成员查询、数组维度和位域解析。
- 示例 ELF/AXF 的变量数量和基础解析结果。

测试依赖示例文件：

- `sample/APP.elf`
- `sample/MC_SixStep_DualLoop.axf`

如果示例文件不存在或不是当前测试期望版本，集成测试可能失败；这不代表模块不能解析其他合法 ELF/AXF。

## 命令行运行

命令格式：

```bash
build/vm_elf_parser_make/vm_elf_parser_cli path/to/file.axf
build/vm_elf_parser_make/vm_elf_parser_cli path/to/file.elf
```

参数说明：

| 参数 | 说明 |
|---|---|
| `file` | 输入 ELF/AXF 文件路径。模块会通过公开变量清单接口加载文件并输出展开后的变量。 |

输出为 CSV 文本，字段如下：

| 字段 | 说明 |
|---|---|
| `name` | 展开后的变量名，例如结构体成员或数组元素。 |
| `address` | MCU 目标地址。 |
| `size` | 读写字节数。 |
| `type` | 解析到的类型名。 |
| `monitorable` | 是否可监控。 |
| `calibratable` | 是否可标定。 |
| `writable` | ELF 符号所在段是否可写。 |
| `bit_field` | 是否为位域。 |
| `bit_offset` | 位域在存储单元中的 bit 偏移。 |
| `bit_size` | 位域 bit 宽度。 |

命令行工具只使用 `include/` 中的公开接口，和 UI 使用同一套变量清单结果。底层符号表、DWARF DIE、适配器注册表等细节属于模块内部实现，不作为命令行参数暴露。
## 嵌入式调用流程

UI 或其他程序推荐使用变量清单接口：

```c
vm_monitor_variable_list_t *list = NULL;
char error[VM_MONITOR_ERROR_MAX];
vm_status_t status;
size_t count;

status = vm_monitor_variables_load("D:/project/output/app.axf",
                                   &list,
                                   error,
                                   sizeof(error));
if (status == VM_OK)
{
    count = vm_monitor_variable_count(list);
    /* 遍历 vm_monitor_variable_at() */
}
vm_monitor_variable_list_destroy(list);
```

返回变量后，UI 按以下方式工作：

1. 用户勾选变量。
2. UI 从 `vm_monitor_variable_t.address` 和 `size` 生成通信模块读请求。
3. 目标板返回原始字节。
4. UI 用 `vm_value_format()` 显示十进制当前值。
5. 如果变量参与曲线，用 `vm_value_to_double()` 转为曲线数值。
6. 用户输入标定目标值后，UI 用 `vm_value_encode()` 得到写入字节。
7. 通信模块按地址写入 MCU 实际变量。

## 输入要求

### ELF/AXF 文件

文件必须满足：

- ELF header 合法。
- 包含 section header 和 section string table。
- 至少包含符号表或 DWARF 调试信息之一。
- 如需结构体、数组、枚举和类型显示，必须包含 `.debug_info`、`.debug_abbrev`、`.debug_str` 等 DWARF 段。
- 变量必须有静态绝对地址，才能出现在监控/标定清单中。

### 类型支持

| 类型 | 监控 | 标定 | 说明 |
|---|---|---|---|
| 有符号整数 | 支持 | 支持 | 十进制显示和输入。 |
| 无符号整数 | 支持 | 支持 | 十进制显示和输入。 |
| `float` | 支持 | 支持 | 十进制小数显示和输入。 |
| `double` | 支持 | 支持 | 十进制小数显示和输入。 |
| 指针 | 支持 | 支持 | 读写指针变量本身保存的地址值，不解引用。 |
| 枚举 | 支持 | 支持 | 当前按底层整数读写。 |
| 结构体 | 展开成员 | 不直接标定 | 成员可监控/标定。 |
| 联合体 | 展开成员 | 不直接标定 | 成员共享地址，调用方需理解覆盖关系。 |
| 数组 | 展开元素 | 不直接标定 | 元素可监控/标定。 |
| 位域 | 支持 | 支持 | 写入时基于当前字节做掩码更新。 |

## 输出说明

### 变量清单输出

`vm_monitor_variables_load()` 的输出是 `vm_monitor_variable_list_t`。每个条目包含：

- 展开后的变量名。
- 类型名。
- 真实地址。
- 读写字节数。
- 是否可监控、可标定。
- 位域信息。

### 值格式化输出

`vm_value_format()` 输出十进制字符串，不输出十六进制字节串。这样 UI、监控表格、标定表格和曲线显示使用统一数值口径。

### 值编码输出

`vm_value_encode()` 输出目标 MCU 内存布局字节。输出字节直接交给通信模块写入变量地址。

## 常见问题

### 加载后显示“未知类型”

通常是输入文件缺少完整 DWARF 调试信息，或变量类型是当前未支持的复杂表达式。请确认编译时打开调试信息，并且没有 strip 输出文件。

### 变量显示成很大的整数

如果 `float` 或有符号类型被识别成未知类型或无符号整数，就会按原始字节解释成大整数。应先检查 `type_name` 是否解析正确，再检查 `vm_value_format()` 是否按对应类型执行。

### 指针能不能监控和标定

可以监控和标定指针变量本身。标定写入的是指针变量保存的地址值，不会写指针指向地址中的数据。如果要改指针指向对象，应选择被指向对象的实际变量或成员。

### 结构体本身为什么不显示成可标定变量

结构体本身是聚合对象，直接写入整块内存风险较高。模块按成员展开，例如 `pid.SetPoint`，只对最细叶子成员进行读写。

## 集成注意事项

- UI 加载新 AXF 后应清空旧变量列表、旧监控选择和旧标定选择，避免旧地址写到新程序。
- 标定位域前必须先读当前存储单元，再调用 `vm_value_encode()`，否则无法保护同一字节里的其他位。
- `vm_monitor_variable_t.address` 是目标地址，不是 PC 进程地址，不能在上位机直接解引用。
- 模块 API 不是多线程同步对象；同一个列表或 graph 建议在一个服务线程内使用，跨线程使用时由调用方加锁。
