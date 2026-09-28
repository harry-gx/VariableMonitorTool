/*
 * 文件说明：DWARF abbrev 表解析实现。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "../private/vm_dwarf_internal.h"
#include <stdlib.h>
#include <string.h>

/**
 * 函数说明：dw_uint，执行本模块对应功能逻辑。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；width：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t dw_uint(dw_cursor *c, unsigned width, uint64_t *out)
{
    vm_status_t s = vm_read_uint(&c->reader, c->pos, width, out);
    if (s == VM_OK)
    {
        c->pos += width;
    }
    return s;
}

/**
 * 函数说明：dw_uleb，执行本模块对应功能逻辑。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t dw_uleb(dw_cursor *c, uint64_t *out)
{
    uint64_t value = 0;
    uint64_t byte;
    unsigned i;
    for (i = 0; i < 10; ++i)
    {
        if (dw_uint(c, 1, &byte) != VM_OK)
        {
            return VM_FORMAT;
        }
        if (i == 9 && (byte & 0xfe))
        {
            return VM_FORMAT;
        }
        value |= (byte & 0x7f) << (7 * i);
        if (!(byte & 0x80))
        {
            *out = value;
            return VM_OK;
        }
    }
    return VM_FORMAT;
}

/**
 * 函数说明：dw_sleb，执行本模块对应功能逻辑。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t dw_sleb(dw_cursor *c, int64_t *out)
{
    uint64_t value = 0;
    uint64_t byte;
    unsigned i;
    unsigned bits;
    for (i = 0; i < 10; ++i)
    {
        if (dw_uint(c, 1, &byte) != VM_OK)
        {
            return VM_FORMAT;
        }
        if (i == 9 && byte != 0 && byte != 0x7f)
        {
            return VM_FORMAT;
        }
        value |= (byte & 0x7f) << (7 * i);
        if (!(byte & 0x80))
        {
            bits = (i + 1) * 7;
            if (bits < 64 && (byte & 0x40))
            {
                value |= UINT64_MAX << bits;
            }
            /* Avoid an out-of-range unsigned-to-signed cast. */
            *out = value <= INT64_MAX ? (int64_t)value
                                      : -1 - (int64_t)(UINT64_MAX - value);
            return VM_OK;
        }
    }
    return VM_FORMAT;
}

/**
 * 函数说明：grow，执行本模块对应功能逻辑。
 * 输入：p：函数输入参数，参与本函数的计算、查找或状态更新。；capacity：函数输入参数，参与本函数的计算、查找或状态更新。；count：函数输入参数，参与本函数的计算、查找或状态更新。；width：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t
grow(void *p, size_t *capacity, size_t count, size_t width, void **out)
{
    size_t n;
    void *q;
    if (count < *capacity)
    {
        *out = p;
        return VM_OK;
    }
    n = *capacity ? *capacity * 2 : 16;
    if (n < *capacity || n > SIZE_MAX / width)
    {
        return VM_NOMEM;
    }
    q = realloc(p, n * width);
    if (!q)
    {
        return VM_NOMEM;
    }
    *out = q;
    *capacity = n;
    return VM_OK;
}

/**
 * 函数说明：dw_table_free，执行本模块对应功能逻辑。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void dw_table_free(dw_table *t)
{
    free(t->entries);
    free(t->attrs);
    memset(t, 0, sizeof(*t));
}

/**
 * 函数说明：by_code，执行本模块对应功能逻辑。
 * 输入：a：函数输入参数，参与本函数的计算、查找或状态更新。；b：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
static int by_code(const void *a, const void *b)
{
    uint64_t x = ((const dw_abbrev *)a)->view.code;
    uint64_t y = ((const dw_abbrev *)b)->view.code;
    return (x > y) - (x < y);
}

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
                          size_t *error_offset)
{
    dw_cursor c = {{bytes, size, 0}, offset};
    dw_table t = {0};
    size_t cap = 0;
    size_t attr_cap = 0;
    size_t i;
    uint64_t code;
    uint64_t tag;
    uint64_t children;
    void *storage;
    vm_status_t s = VM_FORMAT;
    if (!bytes || !out)
    {
        return VM_INVALID;
    }
    for (;;)
    {
        dw_abbrev item;
        if (dw_uleb(&c, &code) != VM_OK)
        {
            goto fail;
        }
        if (!code)
        {
            break;
        }
        if (dw_uleb(&c, &tag) != VM_OK || !tag ||
            dw_uint(&c, 1, &children) != VM_OK || children > 1)
        {
            goto fail;
        }
        memset(&item, 0, sizeof(item));
        item.view.code = code;
        item.view.tag = tag;
        item.view.has_children = (uint8_t)children;
        item.first_attr = t.attr_count;
        for (;;)
        {
            vm_dwarf_attr_form_t af = {0};
            if (dw_uleb(&c, &af.attribute) != VM_OK ||
                dw_uleb(&c, &af.form) != VM_OK)
            {
                goto fail;
            }
            if (!af.attribute && !af.form)
            {
                break;
            }
            if (!af.attribute || !af.form)
            {
                goto fail;
            }
            if (af.form == VM_DW_FORM_implicit_const &&
                dw_sleb(&c, &af.implicit_const) != VM_OK)
            {
                goto fail;
            }
            s = grow(
                t.attrs, &attr_cap, t.attr_count, sizeof(*t.attrs), &storage);
            if (s != VM_OK)
            {
                goto fail;
            }
            t.attrs = storage;
            s = VM_FORMAT;
            t.attrs[t.attr_count++] = af;
            item.view.attributes++;
        }
        s = grow(t.entries, &cap, t.count, sizeof(*t.entries), &storage);
        if (s != VM_OK)
        {
            goto fail;
        }
        t.entries = storage;
        s = VM_FORMAT;
        t.entries[t.count++] = item;
    }
    /* Keep input order for the legacy index-based API; check duplicate codes. */
    if (t.count)
    {
        dw_abbrev *sorted = malloc(t.count * sizeof(*sorted));
        if (!sorted)
        {
            s = VM_NOMEM;
            goto fail;
        }
        memcpy(sorted, t.entries, t.count * sizeof(*sorted));
        qsort(sorted, t.count, sizeof(*sorted), by_code);
        for (i = 1; i < t.count; ++i)
        {
            if (sorted[i - 1].view.code == sorted[i].view.code)
            {
                free(sorted);
                goto fail;
            }
        }
        free(sorted);
    }
    *out = t;
    return VM_OK;
fail:
    if (error_offset)
    {
        *error_offset = c.pos;
    }
    dw_table_free(&t);
    return s;
}

/**
 * 函数说明：dw_table_find，查找匹配对象。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。；code：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
const dw_abbrev *dw_table_find(const dw_table *t, uint64_t code)
{
    size_t i;
    for (i = 0; i < t->count; ++i)
    {
        if (t->entries[i].view.code == code)
        {
            return &t->entries[i];
        }
    }
    return NULL;
}

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
                                      vm_dwarf_abbrev_view_t *out)
{
    dw_table t = {0};
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    s = dw_table_read(bytes, size, offset, &t, NULL);
    if (s != VM_OK)
    {
        return s;
    }
    s = index < t.count ? VM_OK : VM_NOT_FOUND;
    if (s == VM_OK)
    {
        *out = t.entries[index].view;
    }
    dw_table_free(&t);
    return s;
}

/**
 * 函数说明：vm_dwarf_abbrev_at，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_abbrev_at(const void *bytes,
                               size_t size,
                               size_t index,
                               vm_dwarf_abbrev_view_t *out)
{
    return vm_dwarf_abbrev_at_offset(bytes, size, 0, index, out);
}

/**
 * 函数说明：vm_dwarf_abbrev_count_checked，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_abbrev_count_checked(const void *bytes,
                                          size_t size,
                                          size_t offset,
                                          size_t *out)
{
    dw_table t = {0};
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    s = dw_table_read(bytes, size, offset, &t, NULL);
    if (s == VM_OK)
    {
        *out = t.count;
    }
    dw_table_free(&t);
    return s;
}
/**
 * 函数说明：vm_dwarf_abbrev_count_at，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_dwarf_abbrev_count_at(const void *bytes, size_t size, size_t offset)
{
    size_t n = 0;
    (void)vm_dwarf_abbrev_count_checked(bytes, size, offset, &n);
    return n;
}
/**
 * 函数说明：vm_dwarf_abbrev_count，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_dwarf_abbrev_count(const void *bytes, size_t size)
{
    return vm_dwarf_abbrev_count_at(bytes, size, 0);
}

/**
 * 函数说明：vm_dwarf_abbrev_attr_at，执行本模块对应功能逻辑。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；attr：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_dwarf_abbrev_attr_at(const void *bytes,
                                    size_t size,
                                    size_t offset,
                                    size_t index,
                                    size_t attr,
                                    vm_dwarf_attr_form_t *out)
{
    dw_table t = {0};
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    s = dw_table_read(bytes, size, offset, &t, NULL);
    if (s != VM_OK)
    {
        return s;
    }
    s = VM_NOT_FOUND;
    if (index < t.count && attr < t.entries[index].view.attributes)
    {
        *out = t.attrs[t.entries[index].first_attr + attr];
        s = VM_OK;
    }
    dw_table_free(&t);
    return s;
}

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
                                vm_dwarf_die_header_t *out)
{
    dw_cursor c = {{bytes, size, 0}, offset};
    dw_table t = {0};
    vm_dwarf_die_header_t h = {0};
    const dw_abbrev *a;
    vm_status_t s;
    if (!bytes || !abbrev || !out)
    {
        return VM_INVALID;
    }
    if (dw_uleb(&c, &h.abbrev_code) != VM_OK)
    {
        return VM_FORMAT;
    }
    h.offset = offset;
    h.attributes_offset = c.pos;
    if (!h.abbrev_code)
    {
        *out = h;
        return VM_OK;
    }
    s = dw_table_read(abbrev, abbrev_size, abbrev_offset, &t, NULL);
    if (s != VM_OK)
    {
        return s;
    }
    a = dw_table_find(&t, h.abbrev_code);
    s = a ? VM_OK : VM_NOT_FOUND;
    if (a)
    {
        h.tag = a->view.tag;
        h.has_children = a->view.has_children;
        *out = h;
    }
    dw_table_free(&t);
    return s;
}
