#ifndef VM_TYPE_GRAPH_H
#define VM_TYPE_GRAPH_H
#include "vm_dwarf.h"
VM_BEGIN
typedef struct vm_type_graph vm_type_graph_t;
typedef enum
{
    VM_TYPE_UNKNOWN = 0,
    VM_TYPE_BASE,
    VM_TYPE_POINTER,
    VM_TYPE_ARRAY,
    VM_TYPE_STRUCT,
    VM_TYPE_UNION,
    VM_TYPE_ENUM,
    VM_TYPE_ENUMERATOR,
    VM_TYPE_TYPEDEF,
    VM_TYPE_CONST,
    VM_TYPE_VOLATILE,
    VM_TYPE_VARIABLE,
    VM_TYPE_MEMBER,
    VM_TYPE_SUBRANGE
} vm_type_kind_t;
typedef struct
{
    uint64_t die_offset;
    uint64_t type_die_offset;
    uint64_t parent_die_offset;
    const char *name;
    uint64_t byte_size;
    vm_type_kind_t kind;
    uint8_t has_name;
    uint8_t has_type;
    uint8_t has_byte_size;
    uint64_t member_offset;
    uint64_t bit_offset;
    uint64_t bit_size;
    uint64_t location_address;
    int64_t enum_value;
    uint64_t element_count;
    uint8_t has_element_count;
    uint8_t has_stride;
    uint8_t has_member_offset; /* zero offset is valid; unknown is not zero */
    uint8_t has_bit_offset;
    uint8_t has_bit_size;
    uint8_t bit_offset_is_data;
    uint8_t has_location_address;
    uint8_t has_enum_value;
} vm_type_node_view_t;
/* Snapshot owns names and may outlive input ELF/section buffers. All returned
   views remain valid until destroy. Build publishes no graph on failure. */
vm_status_t vm_type_graph_build(const vm_dwarf_sections_t *sections,
                                vm_type_graph_t **out,
                                vm_dwarf_error_t *error);
void vm_type_graph_destroy(vm_type_graph_t *graph);
size_t vm_type_graph_count(const vm_type_graph_t *graph);
vm_status_t vm_type_graph_at(const vm_type_graph_t *graph,
                             size_t index,
                             vm_type_node_view_t *out);
vm_status_t vm_type_graph_find_die(const vm_type_graph_t *graph,
                                   uint64_t die_offset,
                                   vm_type_node_view_t *out);
size_t vm_type_graph_variable_count(const vm_type_graph_t *graph);
size_t vm_type_graph_static_address_count(const vm_type_graph_t *graph);
vm_status_t vm_type_graph_variable_at(const vm_type_graph_t *graph,
                                      size_t index,
                                      vm_type_node_view_t *out);
vm_status_t vm_type_graph_find_variable(const vm_type_graph_t *graph,
                                        const char *name,
                                        vm_type_node_view_t *out);
vm_status_t vm_type_graph_find_variable_by_address(const vm_type_graph_t *graph,
                                                   uint64_t address,
                                                   vm_type_node_view_t *out);
vm_status_t vm_type_graph_find_variable_containing(const vm_type_graph_t *graph,
                                                   uint64_t address,
                                                   vm_type_node_view_t *out);
size_t vm_type_graph_member_count(const vm_type_graph_t *graph,
                                  uint64_t aggregate_die);
vm_status_t vm_type_graph_member_at(const vm_type_graph_t *graph,
                                    uint64_t aggregate_die,
                                    size_t index,
                                    vm_type_node_view_t *out);
vm_status_t vm_type_graph_find_member_containing(const vm_type_graph_t *graph,
                                                 uint64_t aggregate_die,
                                                 uint64_t offset,
                                                 vm_type_node_view_t *out);
vm_status_t vm_type_graph_find_enumerator(const vm_type_graph_t *graph,
                                          uint64_t enum_die,
                                          const char *name,
                                          vm_type_node_view_t *out);
vm_status_t vm_type_graph_find_enumerator_by_value(const vm_type_graph_t *graph,
                                                   uint64_t enum_die,
                                                   int64_t value,
                                                   vm_type_node_view_t *out);
size_t vm_type_graph_enumerator_count(const vm_type_graph_t *graph,
                                      uint64_t enum_die);
vm_status_t vm_type_graph_enumerator_at(const vm_type_graph_t *graph,
                                        uint64_t enum_die,
                                        size_t index,
                                        vm_type_node_view_t *out);
enum
{
    VM_TYPE_QUAL_CONST = 1,
    VM_TYPE_QUAL_VOLATILE = 2
};
typedef struct
{
    vm_type_node_view_t type;
    uint32_t qualifiers;
} vm_type_resolution_t;
/* Follow variable/typedef/const/volatile only. Stop at pointers and aggregates;
   qualifiers describe this level, not a pointer's pointee. Outputs unchanged
   on failure: FORMAT=cycle/invalid edge, NOT_FOUND=missing node,
   UNSUPPORTED=missing type or size. sizeof also infers contiguous fixed-array sizes; overflow returns FORMAT. */
vm_status_t vm_type_graph_resolve(const vm_type_graph_t *graph,
                                  uint64_t die_offset,
                                  vm_type_resolution_t *out);
vm_status_t vm_type_graph_sizeof(const vm_type_graph_t *graph,
                                 uint64_t die_offset,
                                 uint64_t *out);
/* Array DIE (after resolve), direct dimensions in declaration order.
   Unknown/dynamic dimensions remain visible with has_element_count == 0. */
size_t vm_type_graph_dimension_count(const vm_type_graph_t *graph,
                                     uint64_t array_die);
vm_status_t vm_type_graph_dimension_at(const vm_type_graph_t *graph,
                                       uint64_t array_die,
                                       size_t index,
                                       vm_type_node_view_t *out);
vm_status_t vm_type_graph_array_count(const vm_type_graph_t *graph,
                                      uint64_t array_die,
                                      uint64_t *out);
VM_END
#endif
