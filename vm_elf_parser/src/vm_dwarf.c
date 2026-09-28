/*
 * 文件说明：DWARF 总入口实现，负责协调 abbrev、DIE、form 解析结果。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_dwarf.h"
#include "vm_reader.h"
#include <string.h>

/**
 * 函数说明：vm_dwarf_probe_unit_ex，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；big_endian：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_probe_unit_ex(const void *bytes,
                                   size_t size,
                                   int big_endian,
                                   vm_dwarf_unit_header_t *out)
{
    vm_reader_t r;
    vm_dwarf_unit_header_t h;
    uint64_t value;
    size_t prefix = 4;
    size_t cursor;
    size_t offset_width = 4;
    if (!bytes || !out || (big_endian != 0 && big_endian != 1))
    {
        return VM_INVALID;
    }
    memset(&h, 0, sizeof(h));
    r.data = bytes;
    r.size = size;
    r.big_endian = big_endian;
/* 常量说明：READ(at, 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define READ(at, width, target)                                                \
    do                                                                         \
    {                                                                          \
        if (vm_read_uint(&r, (at), (width), &value) != VM_OK)                  \
        {                                                                      \
            return VM_FORMAT;                                                  \
        }                                                                      \
        (target) = value;                                                      \
    } while (0)
    READ(0, 4, h.unit_length);
    if (h.unit_length == UINT32_MAX)
    {
        prefix = 12;
        offset_width = 8;
        h.dwarf64 = 1;
        READ(4, 8, h.unit_length);
    }
    else if (h.unit_length >= 0xfffffff0u)
    {
        return VM_FORMAT;
    }
    if (h.unit_length > size - prefix)
    {
        return VM_FORMAT;
    }
    h.total_size = prefix + (size_t)h.unit_length;
    r.size = h.total_size;
    READ(prefix, 2, h.version);
    if (h.version < 2 || h.version > 5)
    {
        return VM_UNSUPPORTED;
    }
    cursor = prefix + 2;
    if (h.version == 5)
    {
        READ(cursor, 1, h.unit_type);
        READ(cursor + 1, 1, h.address_size);
        cursor += 2;
        READ(cursor, offset_width, h.abbrev_offset);
        cursor += offset_width;
        switch (h.unit_type)
        {
            case 1:
            case 3:
                break;
            case 2:
            case 6:
                READ(cursor, 8, h.type_signature);
                cursor += 8;
                READ(cursor, offset_width, h.type_offset);
                cursor += offset_width;
                break;
            case 4:
            case 5:
                READ(cursor, 8, h.dwo_id);
                cursor += 8;
                break;
            default:
                return VM_UNSUPPORTED;
        }
    }
    else
    {
        h.unit_type = 1;
        READ(cursor, offset_width, h.abbrev_offset);
        cursor += offset_width;
        READ(cursor, 1, h.address_size);
        cursor++;
    }
    if (h.address_size != 4 && h.address_size != 8)
    {
        return VM_UNSUPPORTED;
    }
    if ((h.unit_type == 2 || h.unit_type == 6) &&
        (h.type_offset < cursor || h.type_offset >= h.total_size))
    {
        return VM_FORMAT;
    }
    h.header_size = cursor;
    *out = h;
    return VM_OK;
#undef READ
}
/**
 * 函数说明：vm_dwarf_probe_unit，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_dwarf_probe_unit(const void *bytes, size_t size, vm_dwarf_unit_header_t *out)
{
    return vm_dwarf_probe_unit_ex(bytes, size, 0, out);
}
