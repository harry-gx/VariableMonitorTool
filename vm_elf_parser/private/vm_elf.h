#ifndef VM_ELF_H
#define VM_ELF_H
#include "vm_registry.h"
VM_BEGIN
typedef struct vm_elf vm_elf_t;
typedef struct
{
    const char *name;
    uint64_t address;
    uint64_t size;
    uint32_t section_index;
    uint32_t section_flags;
    uint8_t binding;
} vm_symbol_t;
typedef struct
{
    const char *name;
    uint64_t address;
    uint64_t size;
    uint32_t section_index;
    uint8_t binding;
    uint8_t readable;
    uint8_t writable;
    uint8_t volatile_hint;
} vm_variable_view_t;
typedef struct
{
    uint64_t offset;
    const char *message;
} vm_parse_error_t;
typedef struct
{
    vm_status_t (*parse)(const void *bytes,
                         size_t size,
                         vm_elf_t **out,
                         vm_parse_error_t *error);
    vm_status_t (*parse_file)(const char *path,
                              vm_elf_t **out,
                              vm_parse_error_t *error);
} vm_parser_ops_t;
/* ARM ELF32 executable only in milestone 1. Input copied on success.
   Views remain valid until close. No target writability is implied. */
vm_status_t vm_elf_parse(const void *bytes,
                         size_t size,
                         vm_elf_t **out,
                         vm_parse_error_t *error);
vm_status_t
vm_elf_parse_file(const char *path, vm_elf_t **out, vm_parse_error_t *error);
int vm_elf_has_debug_info(const vm_elf_t *elf);
typedef struct
{
    const uint8_t *data;
    size_t size;
    uint32_t flags;
    int big_endian;
} vm_elf_section_view_t;
/* Borrowed view valid until vm_elf_close; compressed/NOBITS sections unsupported. */
vm_status_t vm_elf_section(const vm_elf_t *elf,
                           const char *name,
                           vm_elf_section_view_t *out);
void vm_elf_close(vm_elf_t *elf);
size_t vm_elf_symbol_count(const vm_elf_t *elf);
vm_status_t
vm_elf_symbol_at(const vm_elf_t *elf, size_t index, vm_symbol_t *out);
size_t vm_elf_variable_count(const vm_elf_t *elf);
vm_status_t
vm_elf_variable_at(const vm_elf_t *elf, size_t index, vm_variable_view_t *out);
vm_status_t vm_elf_find_variable(const vm_elf_t *elf,
                                 const char *name,
                                 vm_variable_view_t *out);
vm_status_t vm_elf_find_variable_by_address(const vm_elf_t *elf,
                                            uint64_t address,
                                            vm_variable_view_t *out);
vm_status_t vm_parser_register_builtins(vm_registry_t *registry);
VM_END
#endif
