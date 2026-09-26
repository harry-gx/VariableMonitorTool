#include "../private/vm_dwarf_internal.h"
#include <stdlib.h>
#include <string.h>

const vm_dwarf_value_t *vm_dwarf_die_attribute(const vm_dwarf_die_view_t *die,
                                               uint64_t attribute)
{
    size_t i;
    if (!die)
    {
        return NULL;
    }
    for (i = 0; i < die->attribute_count; ++i)
    {
        if (die->attributes[i].attribute == attribute)
        {
            return &die->attributes[i].value;
        }
    }
    return NULL;
}

static vm_status_t resolve_value(vm_dwarf_value_t *v,
                                 const vm_dwarf_sections_t *s,
                                 size_t unit,
                                 const vm_dwarf_unit_header_t *h)
{
    vm_dwarf_span_t span;
    const uint8_t *end;
    if (v->kind == VM_DWARF_CU_REFERENCE)
    {
        if (v->unsigned_value < h->header_size ||
            v->unsigned_value >= h->total_size)
        {
            return VM_FORMAT;
        }
        v->unsigned_value += unit;
        v->kind = VM_DWARF_INFO_REFERENCE;
    }
    else if (v->kind == VM_DWARF_INFO_REFERENCE)
    {
        if (v->unsigned_value >= s->info.size)
        {
            return VM_FORMAT;
        }
    }
    else if (v->kind == VM_DWARF_STR_OFFSET ||
             v->kind == VM_DWARF_LINE_STR_OFFSET)
    {
        span = v->kind == VM_DWARF_STR_OFFSET ? s->strings : s->line_strings;
        if (!span.data || v->unsigned_value >= span.size)
        {
            return VM_FORMAT;
        }
        v->data = span.data + (size_t)v->unsigned_value;
        end = memchr(v->data, 0, span.size - (size_t)v->unsigned_value);
        if (!end)
        {
            return VM_FORMAT;
        }
        v->size = (size_t)(end - v->data);
        v->kind = VM_DWARF_STRING;
    }
    return VM_OK;
}

vm_status_t vm_dwarf_walk(const vm_dwarf_sections_t *s,
                          vm_dwarf_die_fn visitor,
                          void *context,
                          vm_dwarf_error_t *error)
{
    size_t unit = 0;
    size_t bad = 0;
    const char *section = ".debug_info";
    const char *message = "invalid unit header";
    uint64_t bad_form = 0;
    dw_table table = {0};
    vm_dwarf_attribute_t *attrs = NULL;
    vm_status_t status = VM_FORMAT;
    if (error)
    {
        memset(error, 0, sizeof(*error));
    }
    if (!s || !visitor || !s->info.data || !s->abbrev.data ||
        (s->big_endian != 0 && s->big_endian != 1))
    {
        return VM_INVALID;
    }
    while (unit < s->info.size)
    {
        vm_dwarf_unit_header_t h;
        vm_dwarf_form_context_t ctx;
        dw_cursor c;
        size_t depth = 0;
        size_t max_attrs = 0;
        size_t i;
        size_t stack_count = 0;
        uint64_t parent_stack[256];
        int seen_root = 0;
        int finished = 0;
        bad = unit;
        section = ".debug_info";
        message = "invalid unit header";
        status = vm_dwarf_probe_unit_ex(
            s->info.data + unit, s->info.size - unit, s->big_endian, &h);
        if (status != VM_OK)
        {
            goto fail;
        }
        if (h.unit_type != 1 && h.unit_type != 3)
        {
            status = VM_UNSUPPORTED;
            message = "non-compilation/partial unit";
            goto fail;
        }
        section = ".debug_abbrev";
        bad = (size_t)h.abbrev_offset;
        message = "invalid abbreviation table";
        if (h.abbrev_offset >= s->abbrev.size)
        {
            status = VM_FORMAT;
            goto fail;
        }
        status = dw_table_read(s->abbrev.data,
                               s->abbrev.size,
                               (size_t)h.abbrev_offset,
                               &table,
                               &bad);
        if (status != VM_OK)
        {
            goto fail;
        }
        for (i = 0; i < table.count; ++i)
        {
            if (table.entries[i].view.attributes > max_attrs)
            {
                max_attrs = table.entries[i].view.attributes;
            }
        }
        if (max_attrs > SIZE_MAX / sizeof(*attrs))
        {
            status = VM_NOMEM;
            goto fail;
        }
        if (max_attrs)
        {
            attrs = malloc(max_attrs * sizeof(*attrs));
            if (!attrs)
            {
                status = VM_NOMEM;
                goto fail;
            }
        }
        ctx.version = h.version;
        ctx.address_size = h.address_size;
        ctx.offset_size = h.dwarf64 ? 8 : 4;
        ctx.big_endian = s->big_endian;
        c.reader.data = s->info.data + unit;
        c.reader.size = h.total_size;
        c.reader.big_endian = s->big_endian;
        c.pos = h.header_size;
        while (c.pos < c.reader.size)
        {
            uint64_t code;
            const dw_abbrev *a;
            vm_dwarf_die_view_t die;
            section = ".debug_info";
            bad = unit + c.pos;
            message = "invalid DIE abbreviation or nesting";
            status = VM_FORMAT;
            bad_form = 0;
            /* ARMCC aligns CUs with zero padding after the completed root. */
            if (finished)
            {
                for (; c.pos < c.reader.size; ++c.pos)
                {
                    if (c.reader.data[c.pos] != 0)
                    {
                        goto fail;
                    }
                }
                break;
            }
            memset(&die, 0, sizeof(die));
            die.offset = unit + c.pos;
            die.unit_offset = unit;
            die.depth = depth;
            if (depth > 256)
            {
                status = VM_FORMAT;
                message = "DIE nesting too deep";
                goto fail;
            }
            die.parent_die_offset = depth ? parent_stack[depth - 1] : 0;
            die.address_size = h.address_size;
            die.big_endian = (uint8_t)s->big_endian;
            if (dw_uleb(&c, &code) != VM_OK)
            {
                goto fail;
            }
            if (!code)
            {
                if (!depth)
                {
                    goto fail;
                }
                if (stack_count)
                {
                    stack_count--;
                }
                if (--depth == 0)
                {
                    finished = 1;
                }
                continue;
            }
            a = dw_table_find(&table, code);
            if (!a)
            {
                goto fail;
            }
            if (!seen_root)
            {
                if (a->view.tag != VM_DW_TAG_compile_unit &&
                    a->view.tag != VM_DW_TAG_partial_unit)
                {
                    goto fail;
                }
                seen_root = 1;
            }
            die.tag = a->view.tag;
            die.has_children = a->view.has_children;
            die.attributes = attrs;
            die.attribute_count = a->view.attributes;
            for (i = 0; i < a->view.attributes; ++i)
            {
                const vm_dwarf_attr_form_t *af =
                    &table.attrs[a->first_attr + i];
                size_t next;
                bad = unit + c.pos;
                bad_form = af->form;
                message = "invalid/unsupported attribute form";
                ctx.implicit_const = af->implicit_const;
                attrs[i].attribute = af->attribute;
                status = vm_dwarf_form_read(c.reader.data,
                                            c.reader.size,
                                            c.pos,
                                            af->form,
                                            &ctx,
                                            &attrs[i].value,
                                            &next);
                if (status != VM_OK)
                {
                    goto fail;
                }
                status = resolve_value(&attrs[i].value, s, unit, &h);
                if (status != VM_OK)
                {
                    message = "invalid string or reference offset";
                    goto fail;
                }
                c.pos = next;
            }
            die.next_offset = unit + c.pos;
            status = visitor(context, &die);
            if (status != VM_OK)
            {
                message = "visitor stopped";
                bad = die.offset;
                goto fail;
            }
            if (a->view.has_children)
            {
                if (stack_count >= 256)
                {
                    status = VM_FORMAT;
                    message = "DIE nesting too deep";
                    goto fail;
                }
                parent_stack[stack_count++] = die.offset;
                depth++;
            }
            else if (depth == 0)
            {
                finished = 1;
            }
        }
        if (!seen_root || !finished || depth != 0)
        {
            status = VM_FORMAT;
            bad = unit + c.pos;
            message = "unterminated DIE tree";
            goto fail;
        }
        free(attrs);
        attrs = NULL;
        dw_table_free(&table);
        unit += h.total_size;
    }
    return VM_OK;
fail:
    if (error)
    {
        error->section = section;
        error->offset = bad;
        error->form = bad_form;
        error->message = message;
    }
    free(attrs);
    dw_table_free(&table);
    return status;
}
