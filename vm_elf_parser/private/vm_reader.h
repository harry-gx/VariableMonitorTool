#ifndef VM_READER_H
#define VM_READER_H
#include "vm_status.h"
VM_BEGIN
typedef struct
{
    const uint8_t *data;
    size_t size;
    int big_endian;
} vm_reader_t;
vm_status_t vm_read_uint(const vm_reader_t *r,
                         size_t offset,
                         unsigned width,
                         uint64_t *out);
int vm_reader_range(const vm_reader_t *r, size_t offset, size_t length);
VM_END
#endif
