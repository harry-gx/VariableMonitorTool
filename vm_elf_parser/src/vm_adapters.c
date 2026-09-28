/*
 * 文件说明：模块适配符号实现，用于满足独立编译时的状态码/接口连接。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_elf.h"
/* Both formats share ELF32 decoding. Compiler-specific DWARF comes later.
   Selection is explicit; an ELF signature cannot distinguish compilers. */
static const vm_parser_ops_t gcc_ops = {vm_elf_parse, vm_elf_parse_file};
static const vm_parser_ops_t armcc_ops = {vm_elf_parse, vm_elf_parse_file};
/**
 * 函数说明：vm_parser_register_builtins，解析输入数据并生成内部结果。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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
