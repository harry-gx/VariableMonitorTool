#ifndef VM_DWARF_H
#define VM_DWARF_H
#include "vm_status.h"
VM_BEGIN
typedef struct
{
    uint64_t unit_length;
    uint8_t dwarf64;
    uint16_t version;
    uint8_t unit_type;
    uint8_t address_size;
    size_t header_size;
    size_t total_size;
    uint64_t abbrev_offset;
    uint64_t type_signature;
    uint64_t type_offset;
    uint64_t dwo_id;
} vm_dwarf_unit_header_t;
/* Probe one DWARF .debug_info unit. Bytes are not retained. */
vm_status_t vm_dwarf_probe_unit(const void *bytes,
                                size_t size,
                                vm_dwarf_unit_header_t *out);
vm_status_t vm_dwarf_probe_unit_ex(const void *bytes,
                                   size_t size,
                                   int big_endian,
                                   vm_dwarf_unit_header_t *out);
typedef struct
{
    uint64_t code;
    uint64_t tag;
    uint8_t has_children;
    size_t attributes;
} vm_dwarf_abbrev_view_t;
typedef struct
{
    uint64_t attribute;
    uint64_t form;
    int64_t implicit_const;
} vm_dwarf_attr_form_t;
typedef struct
{
    size_t offset;
    size_t attributes_offset;
    uint64_t abbrev_code;
    uint64_t tag;
    uint8_t has_children;
} vm_dwarf_die_header_t;
vm_status_t vm_dwarf_abbrev_at(const void *bytes,
                               size_t size,
                               size_t index,
                               vm_dwarf_abbrev_view_t *out);
size_t vm_dwarf_abbrev_count(const void *bytes, size_t size);
vm_status_t vm_dwarf_abbrev_at_offset(const void *bytes,
                                      size_t size,
                                      size_t offset,
                                      size_t index,
                                      vm_dwarf_abbrev_view_t *out);
size_t vm_dwarf_abbrev_count_at(const void *bytes, size_t size, size_t offset);
/* Preferred counting API: malformed tables must not look like a short valid table. */
vm_status_t vm_dwarf_abbrev_count_checked(const void *bytes,
                                          size_t size,
                                          size_t offset,
                                          size_t *out);
vm_status_t vm_dwarf_abbrev_attr_at(const void *bytes,
                                    size_t size,
                                    size_t offset,
                                    size_t abbrev_index,
                                    size_t attr_index,
                                    vm_dwarf_attr_form_t *out);
vm_status_t vm_dwarf_die_header(const void *bytes,
                                size_t size,
                                size_t offset,
                                const void *abbrev,
                                size_t abbrev_size,
                                size_t abbrev_offset,
                                vm_dwarf_die_header_t *out);
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
typedef struct
{
    uint64_t form;
    vm_dwarf_value_kind_t kind;
    uint64_t unsigned_value;
    int64_t signed_value;
    const uint8_t
        *data; /* borrowed string/block, length excludes string terminator */
    size_t size;
} vm_dwarf_value_t;
typedef struct
{
    uint16_t version;
    uint8_t address_size;
    uint8_t offset_size;
    int big_endian;
    int64_t implicit_const; /* supplied by abbreviation for form 0x21 */
} vm_dwarf_form_context_t;
/* Buffer must end at current CU boundary; output/next unchanged on failure. */
vm_status_t vm_dwarf_form_read(const void *bytes,
                               size_t size,
                               size_t offset,
                               uint64_t form,
                               const vm_dwarf_form_context_t *context,
                               vm_dwarf_value_t *out,
                               size_t *next);
/* Legacy shorthand: DWARF4, DWARF32, little endian. */
vm_status_t vm_dwarf_form_skip(const void *bytes,
                               size_t size,
                               size_t offset,
                               uint64_t form,
                               uint8_t address_size,
                               size_t *next);

typedef struct
{
    const uint8_t *data;
    size_t size;
} vm_dwarf_span_t;
typedef struct
{
    vm_dwarf_span_t info;
    vm_dwarf_span_t abbrev;
    vm_dwarf_span_t strings;
    vm_dwarf_span_t line_strings;
    int big_endian;
} vm_dwarf_sections_t;
typedef struct
{
    uint64_t attribute;
    vm_dwarf_value_t value;
} vm_dwarf_attribute_t;
typedef struct
{
    size_t unit_offset;
    size_t offset;
    size_t next_offset;
    size_t depth;
    uint64_t parent_die_offset;
    uint64_t tag;
    uint8_t has_children;
    uint8_t address_size;
    uint8_t big_endian;
    const vm_dwarf_attribute_t *attributes; /* callback lifetime only */
    size_t attribute_count;
} vm_dwarf_die_view_t;
typedef struct
{
    const char *section;
    size_t offset;
    uint64_t form;
    const char *message;
} vm_dwarf_error_t;
typedef vm_status_t (*vm_dwarf_die_fn)(void *context,
                                       const vm_dwarf_die_view_t *die);
/* Stream all CUs. Null DIEs are consumed, not emitted. Callbacks may have run
   before a later error; callers requiring atomic snapshots must stage results.
   Strings/blocks borrow input buffers, attribute arrays live only in callback.
   CU references are normalized to .debug_info offsets; DIE target not resolved. */
vm_status_t vm_dwarf_walk(const vm_dwarf_sections_t *sections,
                          vm_dwarf_die_fn visitor,
                          void *context,
                          vm_dwarf_error_t *error);
const vm_dwarf_value_t *vm_dwarf_die_attribute(const vm_dwarf_die_view_t *die,
                                               uint64_t attribute);
/* Only DW_OP_addr (one absolute address) is evaluated; all other expressions
   stay raw and return unsupported. No location-list or runtime evaluation. */
vm_status_t vm_dwarf_location_address(const vm_dwarf_value_t *value,
                                      uint8_t address_size,
                                      int big_endian,
                                      uint64_t *out);
/* Constant nonnegative offset or a single DW_OP_plus_uconst expression. */
vm_status_t vm_dwarf_member_offset(const vm_dwarf_value_t *value,
                                   uint64_t *out);
VM_END
#endif
