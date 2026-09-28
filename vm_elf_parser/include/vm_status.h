/*
 * 文件说明：ELF/AXF 解析模块统一状态码和 C/C++ 兼容宏定义。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_STATUS_H
#define VM_STATUS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
#define VM_BEGIN                                                               \
    extern "C"                                                                 \
    {
#define VM_END }
#else
#define VM_BEGIN
#define VM_END
#endif
/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
typedef enum
{
    VM_OK = 0,
    VM_INVALID,
    VM_NOMEM,
    VM_IO,
    VM_FORMAT,
    VM_UNSUPPORTED,
    VM_NOT_FOUND,
    VM_DUPLICATE,
    VM_BUSY,
    VM_AMBIGUOUS
} vm_status_t;
#endif
