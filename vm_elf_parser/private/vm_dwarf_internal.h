/*
 * 文件说明：ELF/AXF 解析模块内部接口声明，供模块内部源文件使用。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef DWARF_INTERNAL_H
#define DWARF_INTERNAL_H
#include "vm_dwarf.h"
#include "vm_reader.h"
#include "vm_dwarf_values.h"

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：reader，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_reader_t reader;
    /* 变量说明：pos，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t pos;
} dw_cursor;
/**
 * 函数说明：dw_uint，执行本模块对应功能逻辑。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；width：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t dw_uint(dw_cursor *c, unsigned width, uint64_t *out);
/**
 * 函数说明：dw_uleb，执行本模块对应功能逻辑。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t dw_uleb(dw_cursor *c, uint64_t *out);
/**
 * 函数说明：dw_sleb，执行本模块对应功能逻辑。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t dw_sleb(dw_cursor *c, int64_t *out);

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：view，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_abbrev_view_t view;
    /* 变量说明：first_attr，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t first_attr;
} dw_abbrev;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：entries，保存当前对象运行所需的状态、参数或缓存数据。 */
    dw_abbrev *entries;
    /* 变量说明：attrs，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_dwarf_attr_form_t *attrs;
    /* 变量说明：count，当前元素数量。 */
    size_t count;
    /* 变量说明：attr_count，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t attr_count;
} dw_table;
/**
 * 函数说明：dw_table_read，读取数据或发起读取请求。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。；error_offset：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t dw_table_read(const void *bytes,
                          size_t size,
                          size_t offset,
                          dw_table *out,
                          size_t *error_offset);
/**
 * 函数说明：dw_table_free，执行本模块对应功能逻辑。
 * 输入：table：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void dw_table_free(dw_table *table);
/**
 * 函数说明：dw_table_find，查找匹配对象。
 * 输入：table：函数输入参数，参与本函数的计算、查找或状态更新。；code：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
const dw_abbrev *dw_table_find(const dw_table *table, uint64_t code);
#endif
