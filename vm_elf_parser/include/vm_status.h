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
