/*
 * 文件说明：ELF/AXF 解析模块内部接口声明，供模块内部源文件使用。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_ELF_H
#define VM_ELF_H
#include "vm_registry.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_elf vm_elf_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：name，变量名、页面名或节点名。 */
    const char *name;
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint64_t address;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint64_t size;
    /* 变量说明：section_index，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t section_index;
    /* 变量说明：section_flags，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t section_flags;
    /* 变量说明：binding，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t binding;
} vm_symbol_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：name，变量名、页面名或节点名。 */
    const char *name;
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint64_t address;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint64_t size;
    /* 变量说明：section_index，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t section_index;
    /* 变量说明：binding，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t binding;
    /* 变量说明：readable，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t readable;
    /* 变量说明：writable，变量是否允许写入。 */
    uint8_t writable;
    /* 变量说明：volatile_hint，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t volatile_hint;
} vm_variable_view_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t offset;
    /* 变量说明：message，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *message;
} vm_parse_error_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*parse)(const void *bytes,
                         size_t size,
                         vm_elf_t **out,
                         vm_parse_error_t *error);
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：error：错误信息输出缓冲区。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*parse_file)(const char *path,
                              vm_elf_t **out,
                              vm_parse_error_t *error);
} vm_parser_ops_t;
/**`n * 说明：当前解析 ARM ELF32 可执行文件；成功后会拷贝输入数据，返回的视图在 vm_elf_close 前有效，不隐含目标地址可写。`n * 函数说明：vm_elf_parse，解析输入数据并生成内部结果。`n * 输入：bytes：ELF/AXF 文件原始字节缓冲区；size：输入缓冲区长度；out：解析成功后的 ELF 对象输出指针；error：解析失败时的错误位置和错误文本。`n * 输出：成功时创建 vm_elf_t 对象并写入 out，失败时写入 error。`n * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、格式不支持或内存不足。`n */
vm_status_t vm_elf_parse(const void *bytes,
                         size_t size,
                         vm_elf_t **out,
                         vm_parse_error_t *error);
/**
 * 函数说明：vm_elf_parse_file，解析输入数据并生成内部结果。
 * 输入：path：待加载的文件路径。；out：输出对象或结果指针，函数成功时写入有效值。；error：错误信息输出缓冲区。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_elf_parse_file(const char *path, vm_elf_t **out, vm_parse_error_t *error);
/**
 * 函数说明：vm_elf_has_debug_info，执行本模块对应功能逻辑。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
int vm_elf_has_debug_info(const vm_elf_t *elf);
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：data，保存当前对象运行所需的状态、参数或缓存数据。 */
    const uint8_t *data;
    /* 变量说明：size，数据长度，单位为字节。 */
    size_t size;
    /* 变量说明：flags，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t flags;
    /* 变量说明：big_endian，保存当前对象运行所需的状态、参数或缓存数据。 */
    int big_endian;
} vm_elf_section_view_t;
/* Borrowed view valid until vm_elf_close; compressed/NOBITS sections unsupported. */
/**
 * 函数说明：vm_elf_section，执行本模块对应功能逻辑。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_elf_section(const vm_elf_t *elf,
                           const char *name,
                           vm_elf_section_view_t *out);
/**
 * 函数说明：vm_elf_close，关闭底层资源。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_elf_close(vm_elf_t *elf);
/**
 * 函数说明：vm_elf_symbol_count，执行本模块对应功能逻辑。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_elf_symbol_count(const vm_elf_t *elf);
/**
 * 函数说明：vm_elf_symbol_at，执行本模块对应功能逻辑。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_elf_symbol_at(const vm_elf_t *elf, size_t index, vm_symbol_t *out);
/**
 * 函数说明：vm_elf_variable_count，执行本模块对应功能逻辑。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_elf_variable_count(const vm_elf_t *elf);
/**
 * 函数说明：vm_elf_variable_at，执行本模块对应功能逻辑。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_elf_variable_at(const vm_elf_t *elf, size_t index, vm_variable_view_t *out);
/**
 * 函数说明：vm_elf_find_variable，查找匹配对象。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_elf_find_variable(const vm_elf_t *elf,
                                 const char *name,
                                 vm_variable_view_t *out);
/**
 * 函数说明：vm_elf_find_variable_by_address，查找匹配对象。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_elf_find_variable_by_address(const vm_elf_t *elf,
                                            uint64_t address,
                                            vm_variable_view_t *out);
/**
 * 函数说明：vm_parser_register_builtins，解析输入数据并生成内部结果。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_parser_register_builtins(vm_registry_t *registry);
VM_END
#endif
