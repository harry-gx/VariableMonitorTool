#include "vm_reader.h"
int vm_reader_range(const vm_reader_t *r, size_t o, size_t n)
{
    return r && (r->data || !r->size) && o <= r->size && n <= r->size - o;
}
vm_status_t
vm_read_uint(const vm_reader_t *r, size_t o, unsigned w, uint64_t *out)
{
    uint64_t v = 0;
    unsigned i;
    if (!out || (w != 1 && w != 2 && w != 4 && w != 8))
    {
        return VM_INVALID;
    }
    if (!vm_reader_range(r, o, w))
    {
        return VM_FORMAT;
    }
    for (i = 0; i < w; i++)
    {
        v |= (uint64_t)r->data[o + i] << (8 * (r->big_endian ? w - 1 - i : i));
    }
    *out = v;
    return VM_OK;
}
