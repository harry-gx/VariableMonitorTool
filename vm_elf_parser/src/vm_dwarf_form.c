/*
 * 文件说明：DWARF 属性 form 解码实现。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "../private/vm_dwarf_internal.h"
#include <string.h>

/**
 * 函数说明：vm_dwarf_form_read，读取数据或发起读取请求。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；form：函数输入参数，参与本函数的计算、查找或状态更新。；ctx：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。；next：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_form_read(const void *bytes,
                               size_t size,
                               size_t offset,
                               uint64_t form,
                               const vm_dwarf_form_context_t *ctx,
                               vm_dwarf_value_t *out,
                               size_t *next)
{
    dw_cursor c = {{bytes, size, 0}, offset};
    vm_dwarf_value_t v = {0};
    uint64_t n = 0;
    unsigned width = 0;
    unsigned indirections = 0;
    vm_status_t s;
    const uint8_t *end;
    if (!bytes || !out || !next || !ctx || ctx->version < 2 ||
        ctx->version > 5 ||
        (ctx->address_size != 4 && ctx->address_size != 8) ||
        (ctx->offset_size != 4 && ctx->offset_size != 8) ||
        (ctx->big_endian != 0 && ctx->big_endian != 1))
    {
        return VM_INVALID;
    }
    if (offset > size)
    {
        return VM_FORMAT;
    }
    c.reader.big_endian = ctx->big_endian;
    while (form == VM_DW_FORM_indirect)
    {
        if (++indirections > 16)
        {
            return VM_UNSUPPORTED;
        }
        if (dw_uleb(&c, &form) != VM_OK)
        {
            return VM_FORMAT;
        }
        /* implicit_const's operand belongs in abbrev, not the DIE stream. */
        if (form == VM_DW_FORM_implicit_const)
        {
            return VM_FORMAT;
        }
    }
    v.form = form;
    v.kind = VM_DWARF_UNSIGNED;
    switch (form)
    {
        case VM_DW_FORM_addr:
            width = ctx->address_size;
            v.kind = VM_DWARF_ADDRESS;
            break;
        case VM_DW_FORM_data1:
        case VM_DW_FORM_flag:
            width = 1;
            break;
        case VM_DW_FORM_data2:
            width = 2;
            break;
        case VM_DW_FORM_data4:
            width = 4;
            break;
        case VM_DW_FORM_data8:
            width = 8;
            break;
        case VM_DW_FORM_ref1:
            width = 1;
            v.kind = VM_DWARF_CU_REFERENCE;
            break;
        case VM_DW_FORM_ref2:
            width = 2;
            v.kind = VM_DWARF_CU_REFERENCE;
            break;
        case VM_DW_FORM_ref4:
            width = 4;
            v.kind = VM_DWARF_CU_REFERENCE;
            break;
        case VM_DW_FORM_ref8:
            width = 8;
            v.kind = VM_DWARF_CU_REFERENCE;
            break;
        case VM_DW_FORM_ref_addr:
            width = ctx->version == 2 ? ctx->address_size : ctx->offset_size;
            v.kind = VM_DWARF_INFO_REFERENCE;
            break;
        case VM_DW_FORM_sec_offset:
            width = ctx->offset_size;
            v.kind = VM_DWARF_SECTION_OFFSET;
            break;
        case VM_DW_FORM_strp:
            width = ctx->offset_size;
            v.kind = VM_DWARF_STR_OFFSET;
            break;
        case VM_DW_FORM_line_strp:
            width = ctx->offset_size;
            v.kind = VM_DWARF_LINE_STR_OFFSET;
            break;
        case VM_DW_FORM_ref_sig8:
            width = 8;
            v.kind = VM_DWARF_SIGNATURE;
            break;
        case VM_DW_FORM_flag_present:
            v.unsigned_value = 1;
            break;
        case VM_DW_FORM_implicit_const:
            v.kind = VM_DWARF_SIGNED;
            v.signed_value = ctx->implicit_const;
            break;
        case VM_DW_FORM_sdata:
            v.kind = VM_DWARF_SIGNED;
            if (dw_sleb(&c, &v.signed_value) != VM_OK)
            {
                return VM_FORMAT;
            }
            break;
        case VM_DW_FORM_udata:
        case VM_DW_FORM_ref_udata:
            if (dw_uleb(&c, &v.unsigned_value) != VM_OK)
            {
                return VM_FORMAT;
            }
            if (form == VM_DW_FORM_ref_udata)
            {
                v.kind = VM_DWARF_CU_REFERENCE;
            }
            break;
        case VM_DW_FORM_string:
            end = memchr(c.reader.data + c.pos, 0, size - c.pos);
            if (!end)
            {
                return VM_FORMAT;
            }
            v.kind = VM_DWARF_STRING;
            v.data = c.reader.data + c.pos;
            v.size = (size_t)(end - v.data);
            c.pos += v.size + 1;
            break;
        case VM_DW_FORM_block1:
        case VM_DW_FORM_block2:
        case VM_DW_FORM_block4:
        case VM_DW_FORM_block:
        case VM_DW_FORM_exprloc:
        case VM_DW_FORM_data16:
            if (form == VM_DW_FORM_data16)
            {
                n = 16;
            }
            else
            {
                if (form == VM_DW_FORM_block1)
                {
                    s = dw_uint(&c, 1, &n);
                }
                else if (form == VM_DW_FORM_block2)
                {
                    s = dw_uint(&c, 2, &n);
                }
                else if (form == VM_DW_FORM_block4)
                {
                    s = dw_uint(&c, 4, &n);
                }
                else
                {
                    s = dw_uleb(&c, &n);
                }
                if (s != VM_OK)
                {
                    return VM_FORMAT;
                }
            }
            if (n > size - c.pos)
            {
                return VM_FORMAT;
            }
            v.kind = VM_DWARF_BLOCK;
            v.data = c.reader.data + c.pos;
            v.size = (size_t)n;
            c.pos += v.size;
            break;
        default:
            return VM_UNSUPPORTED;
    }
    if (width && dw_uint(&c, width, &v.unsigned_value) != VM_OK)
    {
        return VM_FORMAT;
    }
    *out = v;
    *next = c.pos;
    return VM_OK;
}

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
                               size_t *next)
{
    vm_dwarf_form_context_t ctx = {4, 0, 4, 0, 0};
    vm_dwarf_value_t value;
    ctx.address_size = address_size;
    return vm_dwarf_form_read(bytes, size, offset, form, &ctx, &value, next);
}

/**
 * 函数说明：vm_dwarf_location_address，执行本模块对应功能逻辑。
 * 输入：v：函数输入参数，参与本函数的计算、查找或状态更新。；address_size：函数输入参数，参与本函数的计算、查找或状态更新。；big_endian：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_location_address(const vm_dwarf_value_t *v,
                                      uint8_t address_size,
                                      int big_endian,
                                      uint64_t *out)
{
    vm_reader_t r;
    if (!v || !out || (address_size != 4 && address_size != 8) ||
        (big_endian != 0 && big_endian != 1))
    {
        return VM_INVALID;
    }
    if (v->kind != VM_DWARF_BLOCK || !v->data ||
        v->size != (size_t)address_size + 1 || v->data[0] != 0x03)
    {
        return VM_UNSUPPORTED;
    }
    r.data = v->data;
    r.size = v->size;
    r.big_endian = big_endian;
    return vm_read_uint(&r, 1, address_size, out);
}

/**
 * 函数说明：vm_dwarf_member_offset，执行本模块对应功能逻辑。
 * 输入：v：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_member_offset(const vm_dwarf_value_t *v, uint64_t *out)
{
    dw_cursor c;
    uint64_t offset;
    if (!v || !out)
    {
        return VM_INVALID;
    }
    if (v->kind == VM_DWARF_UNSIGNED)
    {
        *out = v->unsigned_value;
        return VM_OK;
    }
    if (v->kind == VM_DWARF_SIGNED && v->signed_value >= 0)
    {
        *out = (uint64_t)v->signed_value;
        return VM_OK;
    }
    if (v->kind != VM_DWARF_BLOCK || !v->data || !v->size || v->data[0] != 0x23)
    {
        return VM_UNSUPPORTED;
    }
    c.reader.data = v->data;
    c.reader.size = v->size;
    c.reader.big_endian = 0;
    c.pos = 1;
    if (dw_uleb(&c, &offset) != VM_OK)
    {
        return VM_FORMAT;
    }
    if (c.pos != v->size)
    {
        return VM_UNSUPPORTED;
    }
    *out = offset;
    return VM_OK;
}
