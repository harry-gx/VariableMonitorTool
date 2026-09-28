/*
 * 文件说明：DWARF 类型图构建和类型展开实现，用于还原基础类型、指针、数组、结构体、联合体和枚举。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_type_graph.h"
#include "vm_dwarf_values.h"
#include <stdlib.h>
#include <string.h>

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_type_graph
{
    /* 变量说明：items，动态数组首地址。 */
    vm_type_node_view_t *items;
    /* 变量说明：count，当前元素数量。 */
    size_t count;
    /* 变量说明：capacity，动态数组容量。 */
    size_t capacity;
    /* 变量说明：variables，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t variables;
    /* 变量说明：static_addresses，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t static_addresses;
    /* 变量说明：c_lower_bound，保存当前对象运行所需的状态、参数或缓存数据。 */
    int c_lower_bound;
};

/**
 * 函数说明：node_kind，执行本模块对应功能逻辑。
 * 输入：tag：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static vm_type_kind_t node_kind(uint64_t tag)
{
    switch (tag)
    {
        case VM_DW_TAG_subrange_type:
            return VM_TYPE_SUBRANGE;
        case VM_DW_TAG_base_type:
            return VM_TYPE_BASE;
        case VM_DW_TAG_pointer_type:
            return VM_TYPE_POINTER;
        case VM_DW_TAG_array_type:
            return VM_TYPE_ARRAY;
        case VM_DW_TAG_structure_type:
            return VM_TYPE_STRUCT;
        case VM_DW_TAG_union_type:
            return VM_TYPE_UNION;
        case VM_DW_TAG_enumeration_type:
            return VM_TYPE_ENUM;
        case VM_DW_TAG_enumerator:
            return VM_TYPE_ENUMERATOR;
        case VM_DW_TAG_typedef:
            return VM_TYPE_TYPEDEF;
        case VM_DW_TAG_const_type:
            return VM_TYPE_CONST;
        case VM_DW_TAG_volatile_type:
            return VM_TYPE_VOLATILE;
        case VM_DW_TAG_variable:
            return VM_TYPE_VARIABLE;
        case VM_DW_TAG_member:
            return VM_TYPE_MEMBER;
        default:
            return VM_TYPE_UNKNOWN;
    }
}

/**
 * 函数说明：vm_type_graph_destroy，销毁对象并释放相关资源。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_type_graph_destroy(vm_type_graph_t *g)
{
    size_t i;
    if (!g)
    {
        return;
    }
    for (i = 0; i < g->count; ++i)
    {
        free((void *)g->items[i].name);
    }
    free(g->items);
    free(g);
}

/* Signed bounds outside int64 are deliberately not inferred. */
/**
 * 函数说明：bound，执行本模块对应功能逻辑。
 * 输入：a：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
static int bound(const vm_dwarf_value_t *a, int64_t *out)
{
    if (!a)
    {
        return 0;
    }
    if (a->kind == VM_DWARF_SIGNED)
    {
        *out = a->signed_value;
        return 1;
    }
    if (a->kind == VM_DWARF_UNSIGNED && a->unsigned_value <= INT64_MAX)
    {
        *out = (int64_t)a->unsigned_value;
        return 1;
    }
    return 0;
}

/**
 * 函数说明：collect，执行本模块对应功能逻辑。
 * 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。；die：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t collect(void *context, const vm_dwarf_die_view_t *die)
{
    vm_type_graph_t *g = context;
    vm_type_node_view_t v = {0};
    const vm_dwarf_value_t *a;
    char *name = NULL;
    if (die->depth == 0)
    {
        a = vm_dwarf_die_attribute(die, VM_DW_AT_language);
        g->c_lower_bound =
            a && a->kind == VM_DWARF_UNSIGNED &&
            (a->unsigned_value == 1 || a->unsigned_value == 2 ||
             a->unsigned_value == 0x0c || a->unsigned_value == 0x1d);
    }
    v.kind = node_kind(die->tag);
    if (v.kind == VM_TYPE_UNKNOWN)
    {
        return VM_OK;
    }
    v.has_stride = vm_dwarf_die_attribute(die, VM_DW_AT_byte_stride) != NULL ||
                   vm_dwarf_die_attribute(die, VM_DW_AT_bit_stride) != NULL;
    if (v.kind == VM_TYPE_SUBRANGE)
    {
        int64_t lower = 0;
        int64_t upper;
        a = vm_dwarf_die_attribute(die, VM_DW_AT_count);
        if (a)
        {
            if (a->kind == VM_DWARF_UNSIGNED)
            {
                v.element_count = a->unsigned_value;
                v.has_element_count = 1;
            }
            else if (a->kind == VM_DWARF_SIGNED && a->signed_value >= 0)
            {
                v.element_count = (uint64_t)a->signed_value;
                v.has_element_count = 1;
            }
        }
        else
        {
            const vm_dwarf_value_t *lo =
                vm_dwarf_die_attribute(die, VM_DW_AT_lower_bound);
            a = vm_dwarf_die_attribute(die, VM_DW_AT_upper_bound);
            if ((lo ? bound(lo, &lower) : g->c_lower_bound) && bound(a, &upper))
            {
                if (upper < lower)
                {
                    if (lower != INT64_MIN && upper == lower - 1)
                    {
                        v.has_element_count = 1;
                    }
                }
                else
                {
                    uint64_t delta = (uint64_t)upper - (uint64_t)lower;
                    if (delta != UINT64_MAX)
                    {
                        v.element_count = delta + 1;
                        v.has_element_count = 1;
                    }
                }
            }
        }
    }
    v.die_offset = die->offset;
    v.parent_die_offset = die->parent_die_offset;
    if (v.kind == VM_TYPE_MEMBER)
    {
        a = vm_dwarf_die_attribute(die, VM_DW_AT_data_member_location);
        if (a)
        {
            vm_status_t status = vm_dwarf_member_offset(a, &v.member_offset);
            if (status == VM_OK)
            {
                v.has_member_offset = 1;
            }
            else if (status != VM_UNSUPPORTED)
            {
                return status;
            }
        }
        a = vm_dwarf_die_attribute(die, VM_DW_AT_bit_size);
        if (a && a->kind == VM_DWARF_UNSIGNED)
        {
            v.bit_size = a->unsigned_value;
            v.has_bit_size = 1;
        }
        a = vm_dwarf_die_attribute(die, VM_DW_AT_data_bit_offset);
        if (a && a->kind == VM_DWARF_UNSIGNED)
        {
            v.bit_offset = a->unsigned_value;
            v.has_bit_offset = 1;
            v.bit_offset_is_data = 1;
        }
        else
        {
            a = vm_dwarf_die_attribute(die, VM_DW_AT_bit_offset);
            if (a && a->kind == VM_DWARF_UNSIGNED)
            {
                v.bit_offset = a->unsigned_value;
                v.has_bit_offset = 1;
            }
        }
    }
    if (v.kind == VM_TYPE_VARIABLE)
    {
        a = vm_dwarf_die_attribute(die, VM_DW_AT_location);
        if (a)
        {
            vm_status_t ls = vm_dwarf_location_address(
                a, die->address_size, die->big_endian, &v.location_address);
            if (ls == VM_OK)
            {
                v.has_location_address = 1;
            }
            else if (ls != VM_UNSUPPORTED)
            {
                return ls;
            }
        }
    }
    if (v.kind == VM_TYPE_ENUMERATOR)
    {
        a = vm_dwarf_die_attribute(die, VM_DW_AT_const_value);
        if (a && a->kind == VM_DWARF_SIGNED)
        {
            v.enum_value = a->signed_value;
            v.has_enum_value = 1;
        }
        else if (a && a->kind == VM_DWARF_UNSIGNED &&
                 a->unsigned_value <= INT64_MAX)
        {
            v.enum_value = (int64_t)a->unsigned_value;
            v.has_enum_value = 1;
        }
    }
    a = vm_dwarf_die_attribute(die, VM_DW_AT_type);
    if (a && a->kind == VM_DWARF_INFO_REFERENCE)
    {
        v.type_die_offset = a->unsigned_value;
        v.has_type = 1;
    }
    a = vm_dwarf_die_attribute(die, VM_DW_AT_byte_size);
    if (a && a->kind == VM_DWARF_UNSIGNED)
    {
        v.byte_size = a->unsigned_value;
        v.has_byte_size = 1;
    }
    a = vm_dwarf_die_attribute(die, VM_DW_AT_name);
    if (a && a->kind == VM_DWARF_STRING)
    {
        if (a->size == SIZE_MAX)
        {
            return VM_NOMEM;
        }
        name = malloc(a->size + 1);
        if (!name)
        {
            return VM_NOMEM;
        }
        memcpy(name, a->data, a->size);
        name[a->size] = 0;
        v.name = name;
        v.has_name = 1;
    }
    if (g->count == g->capacity)
    {
        size_t capacity = g->capacity ? g->capacity * 2 : 128;
        vm_type_node_view_t *items;
        if (capacity < g->capacity || capacity > SIZE_MAX / sizeof(*items))
        {
            free(name);
            return VM_NOMEM;
        }
        items = realloc(g->items, capacity * sizeof(*items));
        if (!items)
        {
            free(name);
            return VM_NOMEM;
        }
        g->items = items;
        g->capacity = capacity;
    }
    /* Walker emits ascending .debug_info offsets, enabling binary search. */
    g->items[g->count++] = v;
    if (v.kind == VM_TYPE_VARIABLE)
    {
        g->variables++;
        if (v.has_location_address)
        {
            g->static_addresses++;
        }
    }
    return VM_OK;
}

/**
 * 函数说明：vm_type_graph_build，执行本模块对应功能逻辑。
 * 输入：sections：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。；error：错误信息输出缓冲区。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_build(const vm_dwarf_sections_t *sections,
                                vm_type_graph_t **out,
                                vm_dwarf_error_t *error)
{
    vm_type_graph_t *g;
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    *out = NULL;
    if (!sections)
    {
        return VM_INVALID;
    }
    g = calloc(1, sizeof(*g));
    if (!g)
    {
        return VM_NOMEM;
    }
    s = vm_dwarf_walk(sections, collect, g, error);
    if (s != VM_OK)
    {
        vm_type_graph_destroy(g);
        return s;
    }
    *out = g;
    return VM_OK;
}

/**
 * 函数说明：vm_type_graph_count，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_count(const vm_type_graph_t *g)
{
    return g ? g->count : 0;
}
/**
 * 函数说明：vm_type_graph_variable_count，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_variable_count(const vm_type_graph_t *g)
{
    return g ? g->variables : 0;
}
/**
 * 函数说明：vm_type_graph_static_address_count，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_static_address_count(const vm_type_graph_t *g)
{
    return g ? g->static_addresses : 0;
}

/**
 * 函数说明：vm_type_graph_at，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；i：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_type_graph_at(const vm_type_graph_t *g, size_t i, vm_type_node_view_t *out)
{
    if (!g || !out)
    {
        return VM_INVALID;
    }
    if (i >= g->count)
    {
        return VM_NOT_FOUND;
    }
    *out = g->items[i];
    return VM_OK;
}
/**
 * 函数说明：vm_type_graph_find_die，查找匹配对象。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_die(const vm_type_graph_t *g,
                                   uint64_t offset,
                                   vm_type_node_view_t *out)
{
    size_t lo = 0;
    size_t hi;
    size_t mid;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    hi = g->count;
    while (lo < hi)
    {
        mid = lo + (hi - lo) / 2;
        if (g->items[mid].die_offset < offset)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    if (lo == g->count || g->items[lo].die_offset != offset)
    {
        return VM_NOT_FOUND;
    }
    *out = g->items[lo];
    return VM_OK;
}
/**
 * 函数说明：vm_type_graph_variable_at，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_variable_at(const vm_type_graph_t *g,
                                      size_t index,
                                      vm_type_node_view_t *out)
{
    size_t i;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_VARIABLE && index-- == 0)
        {
            *out = g->items[i];
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_find_variable，查找匹配对象。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_variable(const vm_type_graph_t *g,
                                        const char *name,
                                        vm_type_node_view_t *out)
{
    size_t i;
    if (!g || !name || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_VARIABLE && g->items[i].has_name &&
            strcmp(g->items[i].name, name) == 0)
        {
            *out = g->items[i];
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_find_variable_by_address，查找匹配对象。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_variable_by_address(const vm_type_graph_t *g,
                                                   uint64_t address,
                                                   vm_type_node_view_t *out)
{
    size_t i;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_VARIABLE &&
            g->items[i].has_location_address &&
            g->items[i].location_address == address)
        {
            *out = g->items[i];
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_find_variable_containing，查找匹配对象。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_variable_containing(const vm_type_graph_t *g,
                                                   uint64_t address,
                                                   vm_type_node_view_t *out)
{
    size_t i;
    uint64_t size;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        const vm_type_node_view_t *v = &g->items[i];
        if (v->kind != VM_TYPE_VARIABLE || !v->has_location_address)
        {
            continue;
        }
        if (vm_type_graph_sizeof(g, v->die_offset, &size) != VM_OK)
        {
            continue;
        }
        if (size && address >= v->location_address &&
            address - v->location_address < size)
        {
            *out = *v;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_member_count，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_die：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_member_count(const vm_type_graph_t *g,
                                  uint64_t aggregate_die)
{
    size_t i;
    size_t n = 0;
    if (!g)
    {
        return 0;
    }
    for (i = 0; i < g->count; i++)
    {
        if (g->items[i].kind == VM_TYPE_MEMBER &&
            g->items[i].parent_die_offset == aggregate_die)
        {
            n++;
        }
    }
    return n;
}
/**
 * 函数说明：vm_type_graph_member_at，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_die：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_member_at(const vm_type_graph_t *g,
                                    uint64_t aggregate_die,
                                    size_t index,
                                    vm_type_node_view_t *out)
{
    size_t i;
    size_t n = 0;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; i++)
    {
        if (g->items[i].kind == VM_TYPE_MEMBER &&
            g->items[i].parent_die_offset == aggregate_die)
        {
            if (n++ == index)
            {
                *out = g->items[i];
                return VM_OK;
            }
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_find_member_containing，查找匹配对象。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_die：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_member_containing(const vm_type_graph_t *g,
                                                 uint64_t aggregate_die,
                                                 uint64_t offset,
                                                 vm_type_node_view_t *out)
{
    size_t i;
    uint64_t size;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        const vm_type_node_view_t *m = &g->items[i];
        if (m->kind != VM_TYPE_MEMBER ||
            m->parent_die_offset != aggregate_die || !m->has_member_offset)
        {
            continue;
        }
        if (vm_type_graph_sizeof(g, m->die_offset, &size) != VM_OK || !size)
        {
            continue;
        }
        if (offset >= m->member_offset && offset - m->member_offset < size)
        {
            *out = *m;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_find_enumerator，查找匹配对象。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_enumerator(const vm_type_graph_t *g,
                                          uint64_t enum_die,
                                          const char *name,
                                          vm_type_node_view_t *out)
{
    vm_type_node_view_t parent;
    size_t i;
    if (!g || !name || !out)
    {
        return VM_INVALID;
    }
    if (vm_type_graph_find_die(g, enum_die, &parent) != VM_OK ||
        parent.kind != VM_TYPE_ENUM)
    {
        return VM_NOT_FOUND;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_ENUMERATOR &&
            g->items[i].parent_die_offset == enum_die && g->items[i].has_name &&
            strcmp(g->items[i].name, name) == 0)
        {
            *out = g->items[i];
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_find_enumerator_by_value，查找匹配对象。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。；value：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_enumerator_by_value(const vm_type_graph_t *g,
                                                   uint64_t enum_die,
                                                   int64_t value,
                                                   vm_type_node_view_t *out)
{
    size_t i;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_ENUMERATOR &&
            g->items[i].parent_die_offset == enum_die &&
            g->items[i].has_enum_value && g->items[i].enum_value == value)
        {
            *out = g->items[i];
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_type_graph_enumerator_count，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_enumerator_count(const vm_type_graph_t *g,
                                      uint64_t enum_die)
{
    size_t i;
    size_t count = 0;
    if (!g)
    {
        return 0;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_ENUMERATOR &&
            g->items[i].parent_die_offset == enum_die)
        {
            ++count;
        }
    }
    return count;
}
/**
 * 函数说明：vm_type_graph_enumerator_at，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_enumerator_at(const vm_type_graph_t *g,
                                        uint64_t enum_die,
                                        size_t index,
                                        vm_type_node_view_t *out)
{
    size_t i;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_ENUMERATOR &&
            g->items[i].parent_die_offset == enum_die && index-- == 0)
        {
            *out = g->items[i];
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}

/**
 * 函数说明：transparent，执行本模块对应功能逻辑。
 * 输入：kind：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
static int transparent(vm_type_kind_t kind)
{
    return kind == VM_TYPE_VARIABLE || kind == VM_TYPE_MEMBER ||
           kind == VM_TYPE_TYPEDEF || kind == VM_TYPE_CONST ||
           kind == VM_TYPE_VOLATILE;
}

/**
 * 函数说明：vm_type_graph_resolve，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_resolve(const vm_type_graph_t *g,
                                  uint64_t offset,
                                  vm_type_resolution_t *out)
{
    vm_type_resolution_t result = {0};
    vm_status_t s;
    size_t steps;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (steps = 0; steps < g->count; ++steps)
    {
        s = vm_type_graph_find_die(g, offset, &result.type);
        if (s != VM_OK)
        {
            return s;
        }
        if (steps && (result.type.kind == VM_TYPE_VARIABLE ||
                      result.type.kind == VM_TYPE_MEMBER))
        {
            return VM_FORMAT;
        }
        if (result.type.kind == VM_TYPE_CONST)
        {
            result.qualifiers |= VM_TYPE_QUAL_CONST;
        }
        if (result.type.kind == VM_TYPE_VOLATILE)
        {
            result.qualifiers |= VM_TYPE_QUAL_VOLATILE;
        }
        if (!transparent(result.type.kind))
        {
            *out = result;
            return VM_OK;
        }
        if (!result.type.has_type)
        {
            return VM_UNSUPPORTED;
        }
        offset = result.type.type_die_offset;
    }
    return g->count ? VM_FORMAT : VM_NOT_FOUND;
}

/**
 * 函数说明：vm_type_graph_dimension_count，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；array：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_dimension_count(const vm_type_graph_t *g, uint64_t array)
{
    size_t i;
    size_t count = 0;
    if (!g)
    {
        return 0;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_SUBRANGE &&
            g->items[i].parent_die_offset == array)
        {
            ++count;
        }
    }
    return count;
}

/**
 * 函数说明：vm_type_graph_dimension_at，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；array：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_dimension_at(const vm_type_graph_t *g,
                                       uint64_t array,
                                       size_t index,
                                       vm_type_node_view_t *out)
{
    size_t i;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        if (g->items[i].kind == VM_TYPE_SUBRANGE &&
            g->items[i].parent_die_offset == array && index-- == 0)
        {
            *out = g->items[i];
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}

/**
 * 函数说明：vm_type_graph_array_count，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；array：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_array_count(const vm_type_graph_t *g,
                                      uint64_t array,
                                      uint64_t *out)
{
    vm_type_node_view_t node;
    uint64_t count = 1;
    size_t i;
    size_t dimensions = 0;
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    s = vm_type_graph_find_die(g, array, &node);
    if (s != VM_OK)
    {
        return s;
    }
    if (node.kind != VM_TYPE_ARRAY)
    {
        return VM_INVALID;
    }
    for (i = 0; i < g->count; ++i)
    {
        const vm_type_node_view_t *d = &g->items[i];
        if (d->kind != VM_TYPE_SUBRANGE || d->parent_die_offset != array)
        {
            continue;
        }
        ++dimensions;
        if (!d->has_element_count)
        {
            return VM_UNSUPPORTED;
        }
        if (d->element_count && count > UINT64_MAX / d->element_count)
        {
            return VM_FORMAT;
        }
        count *= d->element_count;
    }
    if (!dimensions)
    {
        return VM_UNSUPPORTED;
    }
    *out = count;
    return VM_OK;
}

/**
 * 函数说明：vm_type_graph_sizeof，执行本模块对应功能逻辑。
 * 输入：g：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_type_graph_sizeof(const vm_type_graph_t *g, uint64_t offset, uint64_t *out)
{
    vm_type_resolution_t resolved;
    vm_status_t s;
    uint64_t factor = 1;
    uint64_t count;
    size_t steps;
    size_t i;
    if (!g || !out)
    {
        return VM_INVALID;
    }
    for (steps = 0; steps < g->count; ++steps)
    {
        if (steps)
        {
            vm_type_node_view_t element;
            s = vm_type_graph_find_die(g, offset, &element);
            if (s != VM_OK)
            {
                return s;
            }
            if (element.kind == VM_TYPE_VARIABLE ||
                element.kind == VM_TYPE_MEMBER)
            {
                return VM_FORMAT;
            }
        }
        s = vm_type_graph_resolve(g, offset, &resolved);
        if (s != VM_OK)
        {
            return s;
        }
        if (resolved.type.has_byte_size)
        {
            if (resolved.type.byte_size &&
                factor > UINT64_MAX / resolved.type.byte_size)
            {
                return VM_FORMAT;
            }
            *out = factor * resolved.type.byte_size;
            return VM_OK;
        }
        if (resolved.type.kind != VM_TYPE_ARRAY || !resolved.type.has_type ||
            resolved.type.has_stride)
        {
            return VM_UNSUPPORTED;
        }
        for (i = 0; i < g->count; ++i)
        {
            if (g->items[i].kind == VM_TYPE_SUBRANGE &&
                g->items[i].parent_die_offset == resolved.type.die_offset &&
                g->items[i].has_stride)
            {
                return VM_UNSUPPORTED;
            }
        }
        s = vm_type_graph_array_count(g, resolved.type.die_offset, &count);
        if (s != VM_OK)
        {
            return s;
        }
        if (count && factor > UINT64_MAX / count)
        {
            return VM_FORMAT;
        }
        factor *= count;
        offset = resolved.type.type_die_offset;
    }
    return g->count ? VM_FORMAT : VM_NOT_FOUND;
}
