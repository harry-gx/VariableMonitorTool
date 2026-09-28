/*
 * 文件说明：监控变量提取主流程，负责从 ELF/DWARF 类型信息展开变量、成员、数组和位字段。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_monitor_variables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm_elf.h"
#include "vm_type_graph.h"

/* 常量说明：VM_MONITOR_MAX_DEPTH 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_MONITOR_MAX_DEPTH (6u)
/* 常量说明：VM_MONITOR_MAX_DIMS 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_MONITOR_MAX_DIMS (8u)
/* 常量说明：VM_MONITOR_INITIAL_CAP 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_MONITOR_INITIAL_CAP (64u)

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：name，变量名、页面名或节点名。 */
    char name[VM_MONITOR_TYPE_NAME_MAX];
    /* 变量说明：kind，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_type_kind_t kind;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint64_t size;
    /* 变量说明：die_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t die_offset;
    /* 变量说明：has_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_size;
    /* 变量说明：is_const，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t is_const;
} vm_resolved_type_info_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_monitor_variable_list
{
    /* 变量说明：items，动态数组首地址。 */
    vm_monitor_variable_t *items;
    /* 变量说明：count，当前元素数量。 */
    size_t count;
    /* 变量说明：capacity，动态数组容量。 */
    size_t capacity;
};

/**
 * 函数说明：vm_monitor_set_error，执行本模块对应功能逻辑。
 * 输入：error：错误信息输出缓冲区。；error_size：错误信息缓冲区长度。；message：协议解析后的消息对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void
vm_monitor_set_error(char *error, size_t error_size, const char *message)
{
    if ((error != NULL) && (error_size > 0u))
    {
        if (message == NULL)
        {
            message = "未知错误";
        }

        (void)snprintf(error, error_size, "%s", message);
        error[error_size - 1u] = '\0';
    }
}

/**
 * 函数说明：vm_monitor_copy_text，复制文本或结构化数据。
 * 输入：dst：函数输入参数，参与本函数的计算、查找或状态更新。；dst_size：函数输入参数，参与本函数的计算、查找或状态更新。；src：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void vm_monitor_copy_text(char *dst, size_t dst_size, const char *src)
{
    size_t index;

    if ((dst == NULL) || (dst_size == 0u))
    {
        return;
    }

    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }

    for (index = 0u; (index + 1u) < dst_size; ++index)
    {
        dst[index] = src[index];
        if (src[index] == '\0')
        {
            return;
        }
    }

    dst[index] = '\0';
}

/**
 * 函数说明：vm_monitor_append_text，追加数据到目标容器。
 * 输入：dst：函数输入参数，参与本函数的计算、查找或状态更新。；dst_size：函数输入参数，参与本函数的计算、查找或状态更新。；src：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t
vm_monitor_append_text(char *dst, size_t dst_size, const char *src)
{
    size_t used;
    size_t index;
    vm_status_t status;

    status = VM_OK;
    if ((dst == NULL) || (src == NULL) || (dst_size == 0u))
    {
        status = VM_INVALID;
    }
    else
    {
        used = strlen(dst);
        if (used >= dst_size)
        {
            status = VM_INVALID;
        }
        else
        {
            for (index = 0u; src[index] != '\0'; ++index)
            {
                if ((used + index + 1u) >= dst_size)
                {
                    status = VM_NOMEM;
                    break;
                }

                dst[used + index] = src[index];
            }

            if (status == VM_OK)
            {
                dst[used + index] = '\0';
            }
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_make_member_name，执行本模块对应功能逻辑。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。；out_size：函数输入参数，参与本函数的计算、查找或状态更新。；parent：Qt 父对象指针。；member：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t vm_monitor_make_member_name(char *out,
                                               size_t out_size,
                                               const char *parent,
                                               const char *member)
{
    vm_status_t status;

    if ((out == NULL) || (parent == NULL) || (member == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        out[0] = '\0';
        status = vm_monitor_append_text(out, out_size, parent);
        if (status == VM_OK)
        {
            status = vm_monitor_append_text(out, out_size, ".");
        }
        if (status == VM_OK)
        {
            status = vm_monitor_append_text(out, out_size, member);
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_make_array_name，执行本模块对应功能逻辑。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。；out_size：函数输入参数，参与本函数的计算、查找或状态更新。；parent：Qt 父对象指针。；index：列表索引或数组下标。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t vm_monitor_make_array_name(char *out,
                                              size_t out_size,
                                              const char *parent,
                                              uint64_t index)
{
    char suffix[32];
    char digits[20];
    uint64_t value;
    size_t digit_count;
    size_t suffix_index;
    vm_status_t status;

    if ((out == NULL) || (parent == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        value = index;
        digit_count = 0u;
        do
        {
            digits[digit_count] = (char)('0' + (char)(value % 10u));
            ++digit_count;
            value /= 10u;
        } while ((value != 0u) && (digit_count < sizeof(digits)));

        suffix_index = 0u;
        suffix[suffix_index] = '[';
        ++suffix_index;
        while ((digit_count > 0u) && ((suffix_index + 2u) < sizeof(suffix)))
        {
            --digit_count;
            suffix[suffix_index] = digits[digit_count];
            ++suffix_index;
        }
        suffix[suffix_index] = ']';
        ++suffix_index;
        suffix[suffix_index] = '\0';

        out[0] = '\0';
        status = vm_monitor_append_text(out, out_size, parent);
        if (status == VM_OK)
        {
            status = vm_monitor_append_text(out, out_size, suffix);
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_is_scalar_type，执行本模块对应功能逻辑。
 * 输入：kind：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t vm_monitor_is_scalar_type(vm_type_kind_t kind)
{
    return (uint8_t)((kind == VM_TYPE_BASE) || (kind == VM_TYPE_ENUM));
}

/**
 * 函数说明：vm_monitor_is_aggregate_type，执行本模块对应功能逻辑。
 * 输入：kind：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t vm_monitor_is_aggregate_type(vm_type_kind_t kind)
{
    return (uint8_t)((kind == VM_TYPE_STRUCT) || (kind == VM_TYPE_UNION));
}

/**
 * 函数说明：vm_monitor_is_array_type，执行本模块对应功能逻辑。
 * 输入：kind：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t vm_monitor_is_array_type(vm_type_kind_t kind)
{
    return (uint8_t)(kind == VM_TYPE_ARRAY);
}

/**
 * 函数说明：vm_monitor_is_expandable_type，执行本模块对应功能逻辑。
 * 输入：kind：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t vm_monitor_is_expandable_type(vm_type_kind_t kind)
{
    return (uint8_t)((vm_monitor_is_aggregate_type(kind) != 0u) ||
                     (vm_monitor_is_array_type(kind) != 0u));
}

/**
 * 函数说明：vm_monitor_reserve，执行本模块对应功能逻辑。
 * 输入：list：变量列表或节点列表对象。；needed：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t vm_monitor_reserve(vm_monitor_variable_list_t *list,
                                      size_t needed)
{
    vm_monitor_variable_t *next_items;
    size_t next_capacity;
    vm_status_t status;

    status = VM_OK;
    if (list == NULL)
    {
        status = VM_INVALID;
    }
    else if (needed > list->capacity)
    {
        next_capacity =
            (list->capacity == 0u) ? VM_MONITOR_INITIAL_CAP : list->capacity;
        while (next_capacity < needed)
        {
            if (next_capacity > (((size_t)-1) / 2u))
            {
                status = VM_NOMEM;
                break;
            }
            next_capacity *= 2u;
        }

        if ((status == VM_OK) &&
            (next_capacity > (((size_t)-1) / sizeof(vm_monitor_variable_t))))
        {
            status = VM_NOMEM;
        }

        if (status == VM_OK)
        {
            next_items = (vm_monitor_variable_t *)realloc(
                list->items, next_capacity * sizeof(vm_monitor_variable_t));
            if (next_items == NULL)
            {
                status = VM_NOMEM;
            }
            else
            {
                list->items = next_items;
                list->capacity = next_capacity;
            }
        }
    }
    else
    {
        status = VM_OK;
    }

    return status;
}

/**
 * 函数说明：vm_monitor_push，执行本模块对应功能逻辑。
 * 输入：list：变量列表或节点列表对象。；item：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t vm_monitor_push(vm_monitor_variable_list_t *list,
                                   const vm_monitor_variable_t *item)
{
    vm_status_t status;

    status = VM_INVALID;
    if ((list != NULL) && (item != NULL))
    {
        status = vm_monitor_reserve(list, list->count + 1u);
        if (status == VM_OK)
        {
            list->items[list->count] = *item;
            list->count++;
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_load_dwarf_sections，加载外部文件或配置。
 * 输入：elf：函数输入参数，参与本函数的计算、查找或状态更新。；sections：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t vm_monitor_load_dwarf_sections(vm_elf_t *elf,
                                              vm_dwarf_sections_t *sections)
{
    vm_elf_section_view_t info;
    vm_elf_section_view_t abbrev;
    vm_elf_section_view_t strings;
    vm_elf_section_view_t line_strings;
    uint8_t result;

    result = 0u;
    if ((elf != NULL) && (sections != NULL))
    {
        (void)memset(sections, 0, sizeof(*sections));
        if ((vm_elf_section(elf, ".debug_info", &info) == VM_OK) &&
            (vm_elf_section(elf, ".debug_abbrev", &abbrev) == VM_OK))
        {
            sections->info.data = info.data;
            sections->info.size = info.size;
            sections->abbrev.data = abbrev.data;
            sections->abbrev.size = abbrev.size;
            sections->big_endian = info.big_endian;

            if (vm_elf_section(elf, ".debug_str", &strings) == VM_OK)
            {
                sections->strings.data = strings.data;
                sections->strings.size = strings.size;
            }

            if (vm_elf_section(elf, ".debug_line_str", &line_strings) == VM_OK)
            {
                sections->line_strings.data = line_strings.data;
                sections->line_strings.size = line_strings.size;
            }

            result = 1u;
        }
    }

    return result;
}

/**
 * 函数说明：vm_monitor_find_debug_variable，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；variable：变量描述信息，包含变量名、类型、地址、长度和权限。；node：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t
vm_monitor_find_debug_variable(const vm_type_graph_t *graph,
                               const vm_variable_view_t *variable,
                               vm_type_node_view_t *node)
{
    uint8_t result;

    result = 0u;
    if ((graph != NULL) && (variable != NULL) && (node != NULL) &&
        (variable->name != NULL))
    {
        if (vm_type_graph_find_variable_by_address(
                graph, variable->address, node) == VM_OK)
        {
            result = 1u;
        }
        else if (vm_type_graph_find_variable(graph, variable->name, node) ==
                 VM_OK)
        {
            result = 1u;
        }
        else
        {
            result = 0u;
        }
    }

    return result;
}

/**
 * 函数说明：vm_monitor_type_name_from_resolved，执行本模块对应功能逻辑。
 * 输入：resolved：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。；out_size：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void vm_monitor_type_name_from_resolved(
    const vm_type_resolution_t *resolved, char *out, size_t out_size)
{
    char base[VM_MONITOR_TYPE_NAME_MAX];

    if ((resolved == NULL) || (out == NULL) || (out_size == 0u))
    {
        return;
    }

    base[0] = '\0';
    out[0] = '\0';

    switch (resolved->type.kind)
    {
        case VM_TYPE_BASE:
            if (resolved->type.has_name != 0u)
            {
                vm_monitor_copy_text(base, sizeof(base), resolved->type.name);
            }
            break;

        case VM_TYPE_ENUM:
            (void)vm_monitor_append_text(base, sizeof(base), "enum");
            if (resolved->type.has_name != 0u)
            {
                (void)vm_monitor_append_text(base, sizeof(base), " ");
                (void)vm_monitor_append_text(
                    base, sizeof(base), resolved->type.name);
            }
            break;

        case VM_TYPE_POINTER:
            vm_monitor_copy_text(base, sizeof(base), "pointer");
            break;

        case VM_TYPE_ARRAY:
            vm_monitor_copy_text(base, sizeof(base), "array");
            break;

        case VM_TYPE_STRUCT:
            (void)vm_monitor_append_text(base, sizeof(base), "struct");
            if (resolved->type.has_name != 0u)
            {
                (void)vm_monitor_append_text(base, sizeof(base), " ");
                (void)vm_monitor_append_text(
                    base, sizeof(base), resolved->type.name);
            }
            break;

        case VM_TYPE_UNION:
            (void)vm_monitor_append_text(base, sizeof(base), "union");
            if (resolved->type.has_name != 0u)
            {
                (void)vm_monitor_append_text(base, sizeof(base), " ");
                (void)vm_monitor_append_text(
                    base, sizeof(base), resolved->type.name);
            }
            break;

        default:
            break;
    }

    if (base[0] != '\0')
    {
        if ((resolved->qualifiers & VM_TYPE_QUAL_CONST) != 0u)
        {
            (void)vm_monitor_append_text(out, out_size, "const ");
        }
        if ((resolved->qualifiers & VM_TYPE_QUAL_VOLATILE) != 0u)
        {
            (void)vm_monitor_append_text(out, out_size, "volatile ");
        }
        (void)vm_monitor_append_text(out, out_size, base);
    }
}

/**
 * 函数说明：vm_monitor_resolve_type_info，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；die_offset：函数输入参数，参与本函数的计算、查找或状态更新。；info：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t vm_monitor_resolve_type_info(const vm_type_graph_t *graph,
                                            uint64_t die_offset,
                                            vm_resolved_type_info_t *info)
{
    vm_type_resolution_t resolved;
    uint64_t byte_size;
    uint8_t result;

    result = 0u;
    if ((graph != NULL) && (info != NULL))
    {
        (void)memset(info, 0, sizeof(*info));
        if (vm_type_graph_resolve(graph, die_offset, &resolved) == VM_OK)
        {
            vm_monitor_type_name_from_resolved(
                &resolved, info->name, sizeof(info->name));
            info->kind = resolved.type.kind;
            info->die_offset = resolved.type.die_offset;
            info->is_const =
                (uint8_t)((resolved.qualifiers & VM_TYPE_QUAL_CONST) != 0u);

            if (vm_type_graph_sizeof(graph, die_offset, &byte_size) == VM_OK)
            {
                info->size = byte_size;
                info->has_size = 1u;
            }
            else if (resolved.type.has_byte_size != 0u)
            {
                info->size = resolved.type.byte_size;
                info->has_size = 1u;
            }
            else
            {
                info->has_size = 0u;
            }

            result = 1u;
        }
    }

    return result;
}

/**
 * 函数说明：vm_monitor_make_variable，执行本模块对应功能逻辑。
 * 输入：name：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；writable：函数输入参数，参与本函数的计算、查找或状态更新。；type_info：函数输入参数，参与本函数的计算、查找或状态更新。；bit_field：函数输入参数，参与本函数的计算、查找或状态更新。；bit_offset：函数输入参数，参与本函数的计算、查找或状态更新。；bit_size：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static vm_monitor_variable_t
vm_monitor_make_variable(const char *name,
                         uint64_t address,
                         uint64_t size,
                         uint8_t writable,
                         const vm_resolved_type_info_t *type_info,
                         uint8_t bit_field,
                         uint8_t bit_offset,
                         uint8_t bit_size)
{
    vm_monitor_variable_t item;

    (void)memset(&item, 0, sizeof(item));
    vm_monitor_copy_text(item.name, sizeof(item.name), name);
    item.address = address;
    item.size = size;
    item.writable = writable;
    item.bit_field = bit_field;
    item.bit_offset = bit_offset;
    item.bit_size = bit_size;

    if (type_info == NULL)
    {
        item.monitorable = (uint8_t)(size > 0u);
        item.calibratable =
            (uint8_t)((item.monitorable != 0u) && (writable != 0u));
    }
    else
    {
        vm_monitor_copy_text(
            item.type_name, sizeof(item.type_name), type_info->name);
        item.monitorable =
            (uint8_t)((size > 0u) &&
                      (vm_monitor_is_scalar_type(type_info->kind) != 0u) &&
                      ((bit_field == 0u) || (bit_size > 0u)));
        item.calibratable =
            (uint8_t)((item.monitorable != 0u) && (writable != 0u) &&
                      (type_info->is_const == 0u));
    }

    return item;
}

static vm_status_t
vm_monitor_append_typed_variable(vm_monitor_variable_list_t *list,
                                 const vm_type_graph_t *graph,
                                 const char *name,
                                 uint64_t address,
                                 uint64_t fallback_size,
                                 uint8_t writable,
                                 uint64_t die_offset,
                                 uint8_t include_self,
                                 uint32_t depth);

/**
 * 函数说明：vm_monitor_array_dimensions，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；array_die：函数输入参数，参与本函数的计算、查找或状态更新。；dimensions：函数输入参数，参与本函数的计算、查找或状态更新。；dimension_count：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t vm_monitor_array_dimensions(const vm_type_graph_t *graph,
                                           uint64_t array_die,
                                           uint64_t *dimensions,
                                           size_t *dimension_count)
{
    size_t count;
    size_t index;
    vm_type_node_view_t dimension;
    uint8_t result;

    result = 0u;
    if ((graph != NULL) && (dimensions != NULL) && (dimension_count != NULL))
    {
        count = vm_type_graph_dimension_count(graph, array_die);
        if ((count > 0u) && (count <= VM_MONITOR_MAX_DIMS))
        {
            result = 1u;
            for (index = 0u; index < count; ++index)
            {
                if ((vm_type_graph_dimension_at(
                         graph, array_die, index, &dimension) != VM_OK) ||
                    (dimension.has_element_count == 0u))
                {
                    result = 0u;
                    break;
                }

                dimensions[index] = dimension.element_count;
            }

            if (result != 0u)
            {
                *dimension_count = count;
            }
        }
    }

    return result;
}

/**
 * 函数说明：vm_monitor_trailing_element_count，执行本模块对应功能逻辑。
 * 输入：dimensions：函数输入参数，参与本函数的计算、查找或状态更新。；dimension_count：函数输入参数，参与本函数的计算、查找或状态更新。；next_dimension：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static uint64_t vm_monitor_trailing_element_count(const uint64_t *dimensions,
                                                  size_t dimension_count,
                                                  size_t next_dimension)
{
    uint64_t count;
    size_t index;

    count = 1u;
    if (dimensions != NULL)
    {
        for (index = next_dimension; index < dimension_count; ++index)
        {
            if ((dimensions[index] == 0u) ||
                (count > (UINT64_MAX / dimensions[index])))
            {
                count = 0u;
                break;
            }

            count *= dimensions[index];
        }
    }

    return count;
}

static vm_status_t
vm_monitor_append_array_elements(vm_monitor_variable_list_t *list,
                                 const vm_type_graph_t *graph,
                                 const char *name,
                                 uint64_t address,
                                 uint8_t writable,
                                 uint64_t element_die,
                                 const uint64_t *dimensions,
                                 size_t dimension_count,
                                 size_t dimension_index,
                                 uint64_t element_size,
                                 uint32_t depth)
{
    vm_status_t status;
    uint64_t count;
    uint64_t trailing;
    uint64_t index;
    uint64_t element_offset;
    char element_name[VM_MONITOR_NAME_MAX];

    status = VM_OK;
    if ((list == NULL) || (graph == NULL) || (name == NULL) ||
        (dimensions == NULL))
    {
        status = VM_INVALID;
    }
    else if (dimension_index >= dimension_count)
    {
        status = vm_monitor_append_typed_variable(list,
                                                  graph,
                                                  name,
                                                  address,
                                                  element_size,
                                                  writable,
                                                  element_die,
                                                  1u,
                                                  depth + 1u);
    }
    else
    {
        count = dimensions[dimension_index];
        trailing = vm_monitor_trailing_element_count(
            dimensions, dimension_count, dimension_index + 1u);
        if ((count != 0u) && (trailing != 0u) && (element_size != 0u))
        {
            for (index = 0u; index < count; ++index)
            {
                if ((trailing > (UINT64_MAX / element_size)) ||
                    (index > (UINT64_MAX / (trailing * element_size))))
                {
                    status = VM_FORMAT;
                    break;
                }

                element_offset = index * trailing * element_size;
                if (vm_monitor_make_array_name(
                        element_name, sizeof(element_name), name, index) !=
                    VM_OK)
                {
                    continue;
                }

                status =
                    vm_monitor_append_array_elements(list,
                                                     graph,
                                                     element_name,
                                                     address + element_offset,
                                                     writable,
                                                     element_die,
                                                     dimensions,
                                                     dimension_count,
                                                     dimension_index + 1u,
                                                     element_size,
                                                     depth + 1u);
                if (status != VM_OK)
                {
                    break;
                }
            }
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_compute_bit_field_access，执行本模块对应功能逻辑。
 * 输入：member：函数输入参数，参与本函数的计算、查找或状态更新。；member_type：函数输入参数，参与本函数的计算、查找或状态更新。；parent_address：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_offset：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；bit_offset：函数输入参数，参与本函数的计算、查找或状态更新。；bit_size：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
static uint8_t
vm_monitor_compute_bit_field_access(const vm_type_node_view_t *member,
                                    const vm_resolved_type_info_t *member_type,
                                    uint64_t parent_address,
                                    uint64_t aggregate_offset,
                                    uint64_t *address,
                                    uint64_t *size,
                                    uint8_t *bit_offset,
                                    uint8_t *bit_size)
{
    uint64_t bit_index;
    uint64_t storage_bits;
    uint64_t byte_offset;
    uint64_t bit_in_byte;
    uint64_t storage_bytes;
    uint8_t result;

    result = 0u;
    if ((member != NULL) && (member_type != NULL) && (address != NULL) &&
        (size != NULL) && (bit_offset != NULL) && (bit_size != NULL) &&
        (member->has_bit_size != 0u) && (member->has_bit_offset != 0u) &&
        (member->bit_size > 0u) && (member->bit_size <= 64u) &&
        (member_type->has_size != 0u) && (member_type->size > 0u) &&
        (member_type->size <= 8u))
    {
        bit_index = 0u;
        if (member->bit_offset_is_data != 0u)
        {
            bit_index = member->bit_offset;
            result = 1u;
        }
        else
        {
            storage_bits = member_type->size * 8u;
            if ((member->bit_offset + member->bit_size) <= storage_bits)
            {
                bit_index = (aggregate_offset * 8u) + storage_bits -
                            member->bit_offset - member->bit_size;
                result = 1u;
            }
        }

        if (result != 0u)
        {
            byte_offset = bit_index / 8u;
            bit_in_byte = bit_index % 8u;
            storage_bytes = (bit_in_byte + member->bit_size + 7u) / 8u;
            if ((storage_bytes > 0u) && (storage_bytes <= 8u) &&
                (bit_in_byte <= UINT8_MAX) && (member->bit_size <= UINT8_MAX))
            {
                *address = parent_address + byte_offset;
                *size = storage_bytes;
                *bit_offset = (uint8_t)bit_in_byte;
                *bit_size = (uint8_t)member->bit_size;
            }
            else
            {
                result = 0u;
            }
        }
    }

    return result;
}

/**
 * 函数说明：vm_monitor_append_aggregate_members，追加数据到目标容器。
 * 输入：list：变量列表或节点列表对象。；graph：函数输入参数，参与本函数的计算、查找或状态更新。；parent_name：函数输入参数，参与本函数的计算、查找或状态更新。；parent_address：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_die：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_kind：函数输入参数，参与本函数的计算、查找或状态更新。；parent_writable：函数输入参数，参与本函数的计算、查找或状态更新。；depth：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t
vm_monitor_append_aggregate_members(vm_monitor_variable_list_t *list,
                                    const vm_type_graph_t *graph,
                                    const char *parent_name,
                                    uint64_t parent_address,
                                    uint64_t aggregate_die,
                                    vm_type_kind_t aggregate_kind,
                                    uint8_t parent_writable,
                                    uint32_t depth)
{
    size_t member_count;
    size_t index;
    vm_type_node_view_t member;
    vm_resolved_type_info_t member_type;
    uint8_t has_member_type;
    uint64_t member_offset;
    uint64_t member_address;
    uint64_t member_size;
    uint8_t member_writable;
    uint64_t bit_address;
    uint64_t bit_storage_size;
    uint8_t bit_offset;
    uint8_t bit_size;
    char member_name[VM_MONITOR_NAME_MAX];
    vm_monitor_variable_t item;
    vm_status_t status;

    status = VM_OK;
    if ((list == NULL) || (graph == NULL) || (parent_name == NULL))
    {
        status = VM_INVALID;
    }
    else if (depth <= VM_MONITOR_MAX_DEPTH)
    {
        member_count = vm_type_graph_member_count(graph, aggregate_die);
        for (index = 0u; index < member_count; ++index)
        {
            if ((vm_type_graph_member_at(
                     graph, aggregate_die, index, &member) != VM_OK) ||
                (member.has_name == 0u))
            {
                continue;
            }

            if (vm_monitor_make_member_name(member_name,
                                            sizeof(member_name),
                                            parent_name,
                                            member.name) != VM_OK)
            {
                continue;
            }

            has_member_type = vm_monitor_resolve_type_info(
                graph, member.die_offset, &member_type);
            member_offset =
                (member.has_member_offset != 0u) ? member.member_offset : 0u;
            member_address = parent_address;
            if (aggregate_kind != VM_TYPE_UNION)
            {
                member_address += member_offset;
            }

            member_size =
                ((has_member_type != 0u) && (member_type.has_size != 0u))
                    ? member_type.size
                    : 0u;
            member_writable = (uint8_t)((parent_writable != 0u) &&
                                        ((has_member_type == 0u) ||
                                         (member_type.is_const == 0u)));

            if ((has_member_type != 0u) && (member.has_bit_size != 0u))
            {
                if (vm_monitor_compute_bit_field_access(
                        &member,
                        &member_type,
                        parent_address,
                        (aggregate_kind == VM_TYPE_UNION) ? 0u : member_offset,
                        &bit_address,
                        &bit_storage_size,
                        &bit_offset,
                        &bit_size) != 0u)
                {
                    item = vm_monitor_make_variable(member_name,
                                                    bit_address,
                                                    bit_storage_size,
                                                    member_writable,
                                                    &member_type,
                                                    1u,
                                                    bit_offset,
                                                    bit_size);
                    status = vm_monitor_push(list, &item);
                }
            }
            else if ((has_member_type != 0u) &&
                     (vm_monitor_is_expandable_type(member_type.kind) != 0u))
            {
                status = vm_monitor_append_typed_variable(list,
                                                          graph,
                                                          member_name,
                                                          member_address,
                                                          member_size,
                                                          member_writable,
                                                          member.die_offset,
                                                          1u,
                                                          depth + 1u);
            }
            else
            {
                item = vm_monitor_make_variable(
                    member_name,
                    member_address,
                    member_size,
                    member_writable,
                    (has_member_type != 0u) ? &member_type : NULL,
                    0u,
                    0u,
                    0u);
                status = vm_monitor_push(list, &item);
            }

            if (status != VM_OK)
            {
                break;
            }
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_append_typed_variable，追加数据到目标容器。
 * 输入：list：变量列表或节点列表对象。；graph：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；fallback_size：函数输入参数，参与本函数的计算、查找或状态更新。；writable：函数输入参数，参与本函数的计算、查找或状态更新。；die_offset：函数输入参数，参与本函数的计算、查找或状态更新。；include_self：函数输入参数，参与本函数的计算、查找或状态更新。；depth：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t
vm_monitor_append_typed_variable(vm_monitor_variable_list_t *list,
                                 const vm_type_graph_t *graph,
                                 const char *name,
                                 uint64_t address,
                                 uint64_t fallback_size,
                                 uint8_t writable,
                                 uint64_t die_offset,
                                 uint8_t include_self,
                                 uint32_t depth)
{
    vm_resolved_type_info_t type_info;
    vm_monitor_variable_t item;
    uint64_t size;
    vm_type_node_view_t array_node;
    uint64_t dimensions[VM_MONITOR_MAX_DIMS];
    size_t dimension_count;
    uint64_t element_size;
    vm_status_t status;

    status = VM_OK;
    if ((list == NULL) || (graph == NULL) || (name == NULL))
    {
        status = VM_INVALID;
    }
    else if (vm_monitor_resolve_type_info(graph, die_offset, &type_info) == 0u)
    {
        item = vm_monitor_make_variable(
            name, address, fallback_size, writable, NULL, 0u, 0u, 0u);
        status = vm_monitor_push(list, &item);
    }
    else
    {
        size = (type_info.has_size != 0u) ? type_info.size : fallback_size;
        if (include_self != 0u)
        {
            item = vm_monitor_make_variable(
                name, address, size, writable, &type_info, 0u, 0u, 0u);
            status = vm_monitor_push(list, &item);
        }

        if ((status == VM_OK) && (depth <= VM_MONITOR_MAX_DEPTH))
        {
            if (vm_monitor_is_aggregate_type(type_info.kind) != 0u)
            {
                status = vm_monitor_append_aggregate_members(
                    list,
                    graph,
                    name,
                    address,
                    type_info.die_offset,
                    type_info.kind,
                    (uint8_t)((writable != 0u) && (type_info.is_const == 0u)),
                    depth + 1u);
            }
            else if (vm_monitor_is_array_type(type_info.kind) != 0u)
            {
                if ((vm_type_graph_find_die(
                         graph, type_info.die_offset, &array_node) == VM_OK) &&
                    (array_node.has_type != 0u) &&
                    (vm_monitor_array_dimensions(graph,
                                                 type_info.die_offset,
                                                 dimensions,
                                                 &dimension_count) != 0u) &&
                    (vm_type_graph_sizeof(graph,
                                          array_node.type_die_offset,
                                          &element_size) == VM_OK) &&
                    (element_size != 0u))
                {
                    status = vm_monitor_append_array_elements(
                        list,
                        graph,
                        name,
                        address,
                        (uint8_t)((writable != 0u) &&
                                  (type_info.is_const == 0u)),
                        array_node.type_die_offset,
                        dimensions,
                        dimension_count,
                        0u,
                        element_size,
                        depth + 1u);
                }
            }
            else
            {
                status = VM_OK;
            }
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_variable_list_destroy，销毁对象并释放相关资源。
 * 输入：list：变量列表或节点列表对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_monitor_variable_list_destroy(vm_monitor_variable_list_t *list)
{
    if (list != NULL)
    {
        free(list->items);
        free(list);
    }
}

/**
 * 函数说明：vm_monitor_variable_count，执行本模块对应功能逻辑。
 * 输入：list：变量列表或节点列表对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_monitor_variable_count(const vm_monitor_variable_list_t *list)
{
    size_t count;

    count = 0u;
    if (list != NULL)
    {
        count = list->count;
    }

    return count;
}

/**
 * 函数说明：vm_monitor_variable_at，执行本模块对应功能逻辑。
 * 输入：list：变量列表或节点列表对象。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_monitor_variable_at(const vm_monitor_variable_list_t *list,
                                   size_t index,
                                   vm_monitor_variable_t *out)
{
    vm_status_t status;

    status = VM_INVALID;
    if ((list != NULL) && (out != NULL))
    {
        if (index < list->count)
        {
            *out = list->items[index];
            status = VM_OK;
        }
        else
        {
            status = VM_NOT_FOUND;
        }
    }

    return status;
}

/**
 * 函数说明：vm_monitor_variables_load，加载外部文件或配置。
 * 输入：path：待加载的文件路径。；out：输出对象或结果指针，函数成功时写入有效值。；error：错误信息输出缓冲区。；error_size：错误信息缓冲区长度。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_monitor_variables_load(const char *path,
                                      vm_monitor_variable_list_t **out,
                                      char *error,
                                      size_t error_size)
{
    vm_status_t status;
    vm_elf_t *elf;
    vm_type_graph_t *type_graph;
    vm_monitor_variable_list_t *list;
    vm_parse_error_t parse_error;
    vm_dwarf_sections_t sections;
    vm_dwarf_error_t dwarf_error;
    size_t index;
    size_t variable_count;
    vm_variable_view_t variable;
    vm_type_node_view_t debug_node;
    vm_monitor_variable_t item;

    status = VM_INVALID;
    elf = NULL;
    type_graph = NULL;
    list = NULL;
    (void)memset(&parse_error, 0, sizeof(parse_error));
    (void)memset(&dwarf_error, 0, sizeof(dwarf_error));

    if (out != NULL)
    {
        *out = NULL;
    }

    if ((path == NULL) || (out == NULL))
    {
        vm_monitor_set_error(error, error_size, "参数错误");
    }
    else if (vm_elf_parse_file(path, &elf, &parse_error) != VM_OK)
    {
        vm_monitor_set_error(error,
                             error_size,
                             (parse_error.message != NULL)
                                 ? parse_error.message
                                 : "ELF/AXF 解析失败");
        status = VM_FORMAT;
    }
    else
    {
        list = (vm_monitor_variable_list_t *)calloc(1u, sizeof(*list));
        if (list == NULL)
        {
            vm_monitor_set_error(error, error_size, "内存不足");
            status = VM_NOMEM;
        }
        else
        {
            if (vm_monitor_load_dwarf_sections(elf, &sections) != 0u)
            {
                /* 关键步骤：用 DWARF 调试节构建类型图，后续变量、结构体成员、数组元素和位字段都从类型图展开。 */
                if (vm_type_graph_build(&sections, &type_graph, &dwarf_error) !=
                    VM_OK)
                {
                    type_graph = NULL;
                }
            }

            variable_count = vm_elf_variable_count(elf);
            status = VM_OK;
            for (index = 0u; index < variable_count; ++index)
            {
                if (vm_elf_variable_at(elf, index, &variable) != VM_OK)
                {
                    continue;
                }

                if (vm_monitor_find_debug_variable(
                        type_graph, &variable, &debug_node) != 0u)
                {
                    status =
                        vm_monitor_append_typed_variable(list,
                                                         type_graph,
                                                         variable.name,
                                                         variable.address,
                                                         variable.size,
                                                         variable.writable,
                                                         debug_node.die_offset,
                                                         1u,
                                                         0u);
                }
                else
                {
                    item = vm_monitor_make_variable(variable.name,
                                                    variable.address,
                                                    variable.size,
                                                    variable.writable,
                                                    NULL,
                                                    0u,
                                                    0u,
                                                    0u);
                    status = vm_monitor_push(list, &item);
                }

                if (status != VM_OK)
                {
                    break;
                }
            }

            if (status == VM_OK)
            {
                *out = list;
                list = NULL;
            }
            else
            {
                vm_monitor_set_error(error, error_size, "变量列表构建失败");
            }
        }
    }

    vm_monitor_variable_list_destroy(list);
    vm_type_graph_destroy(type_graph);
    /* 关键步骤：变量列表已拷贝必要信息，解析结束后释放 ELF 和类型图临时对象。 */
    vm_elf_close(elf);

    return status;
}
