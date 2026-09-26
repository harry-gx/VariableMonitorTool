#ifndef DWARF_INTERNAL_H
#define DWARF_INTERNAL_H
#include "vm_dwarf.h"
#include "vm_reader.h"
#include "vm_dwarf_values.h"

typedef struct
{
    vm_reader_t reader;
    size_t pos;
} dw_cursor;
vm_status_t dw_uint(dw_cursor *c, unsigned width, uint64_t *out);
vm_status_t dw_uleb(dw_cursor *c, uint64_t *out);
vm_status_t dw_sleb(dw_cursor *c, int64_t *out);

typedef struct
{
    vm_dwarf_abbrev_view_t view;
    size_t first_attr;
} dw_abbrev;
typedef struct
{
    dw_abbrev *entries;
    vm_dwarf_attr_form_t *attrs;
    size_t count;
    size_t attr_count;
} dw_table;
vm_status_t dw_table_read(const void *bytes,
                          size_t size,
                          size_t offset,
                          dw_table *out,
                          size_t *error_offset);
void dw_table_free(dw_table *table);
const dw_abbrev *dw_table_find(const dw_table *table, uint64_t code);
#endif
