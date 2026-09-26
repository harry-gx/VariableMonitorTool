#include "vm_elf.h"
/* Both formats share ELF32 decoding. Compiler-specific DWARF comes later.
   Selection is explicit; an ELF signature cannot distinguish compilers. */
static const vm_parser_ops_t gcc_ops = {vm_elf_parse, vm_elf_parse_file};
static const vm_parser_ops_t armcc_ops = {vm_elf_parse, vm_elf_parse_file};
vm_status_t vm_parser_register_builtins(vm_registry_t *r)
{
    const vm_adapter_info_t a = {"arm_gcc_elf", "ARM GCC ELF (symbols)", 1};
    const vm_adapter_info_t b = {
        "keil_armcc_axf", "Keil ARMCC AXF (symbols)", 1};
    vm_status_t s = vm_registry_add(r, &a, &gcc_ops);
    if (s != VM_OK)
    {
        return s;
    }
    s = vm_registry_add(r, &b, &armcc_ops);
    if (s != VM_OK)
    {
        (void)vm_registry_remove(r, a.id);
    }
    return s;
}
