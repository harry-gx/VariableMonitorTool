/*
 * 文件说明：ELF/AXF 解析模块内部接口声明，供模块内部源文件使用。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_DWARF_H
#define VM_DWARF_H
#include "vm_status.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：unit_length，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t unit_length;
    /* 变量说明：dwarf64，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t dwarf64;
    /* 变量说明：version，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint16_t version;
    /* 变量说明：unit_type，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t unit_type;
    /* 变量说明：address_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t address_size;
    /* 变量说明：header_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t header_size;
    /* 变量说明：total_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t total_size;
    /* 变量说明：abbrev_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t abbrev_offset;
    /* 变量说明：type_signature，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t type_signature;
    /* 变量说明：type_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t type_offset;
    /* 变量说明：dwo_id，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t dwo_id;
} vm_dwarf_unit_header_t;
/* Probe one DWARF .debug_info unit. Bytes are not retained. */
/**
 * 函数说明：vm_dwarf_probe_unit，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_probe_unit(const void *bytes,
                                size_t size,
                                vm_dwarf_unit_header_t *out);
/**
 * 函数说明：vm_dwarf_probe_unit_ex，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；big_endian：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_probe_unit_ex(const void *bytes,
                                   size_t size,
                                   int big_endian,
                                   vm_dwarf_unit_header_t *out);
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：code，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t code;
    /* 变量说明：tag，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t tag;
    /* 变量说明：has_children，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_children;
    /* 变量说明：attributes，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t attributes;
} vm_dwarf_abbrev_view_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：attribute，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t attribute;
    /* 变量说明：form，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t form;
    /* 变量说明：implicit_const，保存当前对象运行所需的状态、参数或缓存数据。 */
    int64_t implicit_const;
} vm_dwarf_attr_form_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t offset;
    /* 变量说明：attributes_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t attributes_offset;
    /* 变量说明：abbrev_code，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t abbrev_code;
    /* 变量说明：tag，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t tag;
    /* 变量说明：has_children，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_children;
} vm_dwarf_die_header_t;
/**
 * 函数说明：vm_dwarf_abbrev_at，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_abbrev_at(const void *bytes,
                               size_t size,
                               size_t index,
                               vm_dwarf_abbrev_view_t *out);
/**
 * 函数说明：vm_dwarf_abbrev_count，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_dwarf_abbrev_count(const void *bytes, size_t size);
/**
 * 函数说明：vm_dwarf_abbrev_at_offset，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_abbrev_at_offset(const void *bytes,
                                      size_t size,
                                      size_t offset,
                                      size_t index,
                                      vm_dwarf_abbrev_view_t *out);
/**
 * 函数说明：vm_dwarf_abbrev_count_at，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_dwarf_abbrev_count_at(const void *bytes, size_t size, size_t offset);
/* Preferred counting API: malformed tables must not look like a short valid table. */
/**
 * 函数说明：vm_dwarf_abbrev_count_checked，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_abbrev_count_checked(const void *bytes,
                                          size_t size,
                                          size_t offset,
                                          size_t *out);
/**
 * 函数说明：vm_dwarf_abbrev_attr_at，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；abbrev_index：函数输入参数，参与本函数的计算、查找或状态更新。；attr_index：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_abbrev_attr_at(const void *bytes,
                                    size_t size,
                                    size_t offset,
                                    size_t abbrev_index,
                                    size_t attr_index,
                                    vm_dwarf_attr_form_t *out);
/**
 * 函数说明：vm_dwarf_die_header，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；abbrev：函数输入参数，参与本函数的计算、查找或状态更新。；abbrev_size：函数输入参数，参与本函数的计算、查找或状态更新。；abbrev_offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_die_header(const void *bytes,
                                size_t size,
                                size_t offset,
                                const void *abbrev,
                                size_t abbrev_size,
                                size_t abbrev_offset,
                                vm_dwarf_die_header_t *out);
/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
typedef enum
{
    VM_DWARF_UNSIGNED,
    VM_DWARF_SIGNED,
    VM_DWARF_STRING,
    VM_DWARF_BLOCK,
    VM_DWARF_ADDRESS,
    VM_DWARF_CU_REFERENCE,
    VM_DWARF_INFO_REFERENCE,
    VM_DWARF_SECTION_OFFSET,
    VM_DWARF_STR_OFFSET,
    VM_DWARF_LINE_STR_OFFSET,
    VM_DWARF_SIGNATURE
} vm_dwarf_value_kind_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：form，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t form;
    /* 变量说明：kind，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_value_kind_t kind;
    /* 变量说明：unsigned_value，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t unsigned_value;
    /* 变量说明：signed_value，保存当前对象运行所需的状态、参数或缓存数据。 */
    int64_t signed_value;
    const uint8_t
        *data; /* borrowed string/block, length excludes string terminator */
    /* 变量说明：size，数据长度，单位为字节。 */
    size_t size;
} vm_dwarf_value_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：version，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint16_t version;
    /* 变量说明：address_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t address_size;
    /* 变量说明：offset_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t offset_size;
    /* 变量说明：big_endian，保存当前对象运行所需的状态、参数或缓存数据。 */
    int big_endian;
    int64_t implicit_const; /* supplied by abbreviation for form 0x21 */
} vm_dwarf_form_context_t;
/* Buffer must end at current CU boundary; output/next unchanged on failure. */
/**
 * 函数说明：vm_dwarf_form_read，读取数据或发起读取请求。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；form：函数输入参数，参与本函数的计算、查找或状态更新。；context：回调上下文指针，由调用方传入并在回调中原样返回。；out：输出对象或结果指针，函数成功时写入有效值。；next：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_form_read(const void *bytes,
                               size_t size,
                               size_t offset,
                               uint64_t form,
                               const vm_dwarf_form_context_t *context,
                               vm_dwarf_value_t *out,
                               size_t *next);
/* Legacy shorthand: DWARF4, DWARF32, little endian. */
/**
 * 函数说明：vm_dwarf_form_skip，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；form：函数输入参数，参与本函数的计算、查找或状态更新。；address_size：函数输入参数，参与本函数的计算、查找或状态更新。；next：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_form_skip(const void *bytes,
                               size_t size,
                               size_t offset,
                               uint64_t form,
                               uint8_t address_size,
                               size_t *next);

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：data，保存当前对象运行所需的状态、参数或缓存数据。 */
    const uint8_t *data;
    /* 变量说明：size，数据长度，单位为字节。 */
    size_t size;
} vm_dwarf_span_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：info，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_span_t info;
    /* 变量说明：abbrev，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_span_t abbrev;
    /* 变量说明：strings，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_span_t strings;
    /* 变量说明：line_strings，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_span_t line_strings;
    /* 变量说明：big_endian，保存当前对象运行所需的状态、参数或缓存数据。 */
    int big_endian;
} vm_dwarf_sections_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：attribute，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t attribute;
    /* 变量说明：value，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_value_t value;
} vm_dwarf_attribute_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：unit_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t unit_offset;
    /* 变量说明：offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t offset;
    /* 变量说明：next_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t next_offset;
    /* 变量说明：depth，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t depth;
    /* 变量说明：parent_die_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t parent_die_offset;
    /* 变量说明：tag，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t tag;
    /* 变量说明：has_children，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_children;
    /* 变量说明：address_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t address_size;
    /* 变量说明：big_endian，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t big_endian;
    const vm_dwarf_attribute_t *attributes; /* callback lifetime only */
    /* 变量说明：attribute_count，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t attribute_count;
} vm_dwarf_die_view_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：section，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *section;
    /* 变量说明：offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t offset;
    /* 变量说明：form，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t form;
    /* 变量说明：message，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *message;
} vm_dwarf_error_t;
typedef vm_status_t (*vm_dwarf_die_fn)(void *context,
                                       const vm_dwarf_die_view_t *die);
/**`n * 说明：遍历所有编译单元；空 DIE 只消费不回调，字符串和块数据借用输入缓冲区，属性数组只在回调期间有效。`n * 函数说明：vm_dwarf_walk，遍历 DWARF DIE 并逐个回调调用方。`n * 输入：sections：DWARF 调试节集合；visitor：每个 DIE 的访问回调；context：回调上下文；error：失败时的错误信息输出。`n * 输出：通过 visitor 逐个输出 DIE 视图，出错时写入 error。`n * 返回：返回 VM_OK 表示遍历完成，其它状态码表示格式错误或回调失败。`n */
vm_status_t vm_dwarf_walk(const vm_dwarf_sections_t *sections,
                          vm_dwarf_die_fn visitor,
                          void *context,
                          vm_dwarf_error_t *error);
/**
 * 函数说明：vm_dwarf_die_attribute，执行本模块对应功能逻辑。
 * 输入：die：函数输入参数，参与本函数的计算、查找或状态更新。；attribute：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
const vm_dwarf_value_t *vm_dwarf_die_attribute(const vm_dwarf_die_view_t *die,
                                               uint64_t attribute);
/**`n * 说明：当前只计算单个 DW_OP_addr 绝对地址，其它表达式保持原始数据并返回不支持，不做 location-list 或运行时表达式求值。`n * 函数说明：vm_dwarf_location_address，解析 DWARF location 中的静态地址。`n * 输入：value：DWARF 属性值；address_size：目标地址宽度；big_endian：目标端序；out：解析出的地址输出指针。`n * 输出：成功时写入静态地址。`n * 返回：返回 VM_OK 表示成功，其它状态码表示表达式不支持或输入非法。`n */
vm_status_t vm_dwarf_location_address(const vm_dwarf_value_t *value,
                                      uint8_t address_size,
                                      int big_endian,
                                      uint64_t *out);
/* Constant nonnegative offset or a single DW_OP_plus_uconst expression. */
/**
 * 函数说明：vm_dwarf_member_offset，执行本模块对应功能逻辑。
 * 输入：value：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_member_offset(const vm_dwarf_value_t *value,
                                   uint64_t *out);
VM_END
#endif
