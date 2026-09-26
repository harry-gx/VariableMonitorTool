#include "vm_type_graph.h"
#include "vm_dwarf_values.h"
#include <stdlib.h>
#include <string.h>

struct vm_type_graph
{
    vm_type_node_view_t *items;
    size_t count;
    size_t capacity;
    size_t variables;
    size_t static_addresses;
    int c_lower_bound;
};

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

size_t vm_type_graph_count(const vm_type_graph_t *g)
{
    return g ? g->count : 0;
}
size_t vm_type_graph_variable_count(const vm_type_graph_t *g)
{
    return g ? g->variables : 0;
}
size_t vm_type_graph_static_address_count(const vm_type_graph_t *g)
{
    return g ? g->static_addresses : 0;
}

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

static int transparent(vm_type_kind_t kind)
{
    return kind == VM_TYPE_VARIABLE || kind == VM_TYPE_MEMBER ||
           kind == VM_TYPE_TYPEDEF || kind == VM_TYPE_CONST ||
           kind == VM_TYPE_VOLATILE;
}

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
