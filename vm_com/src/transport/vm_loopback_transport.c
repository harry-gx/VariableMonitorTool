#include "vm_loopback_transport.h"
#include <stdlib.h>
struct vm_loopback_transport
{
    vm_transport_t base;
    vm_transport_rx_fn rx;
    void *context;
};
static vm_status_t open_loop(vm_transport_t *b)
{
    if (!b)
    {
        return VM_INVALID;
    }
    b->opened = 1;
    return VM_OK;
}
static vm_status_t close_loop(vm_transport_t *b)
{
    if (!b)
    {
        return VM_INVALID;
    }
    b->opened = 0;
    return VM_OK;
}
static vm_status_t
send_loop(vm_transport_t *b, const uint8_t *d, size_t n, uint32_t ch)
{
    struct vm_loopback_transport *t;
    if (!b || (!d && n))
    {
        return VM_INVALID;
    }
    if (!b->opened)
    {
        return VM_BUSY;
    }
    t = (struct vm_loopback_transport *)b;
    if (!t->rx)
    {
        return VM_NOT_FOUND;
    }
    {
        vm_transport_packet_t p = {d, n, ch, 0};
        return t->rx(t->context, &p);
    }
}
static vm_status_t set_loop(vm_transport_t *b, vm_transport_rx_fn f, void *c)
{
    struct vm_loopback_transport *t;
    if (!b)
    {
        return VM_INVALID;
    }
    t = (struct vm_loopback_transport *)b;
    t->rx = f;
    t->context = c;
    return VM_OK;
}
static const vm_transport_ops_t ops = {
    open_loop, close_loop, send_loop, set_loop};
vm_status_t vm_loopback_transport_create(vm_loopback_transport_t **out)
{
    vm_loopback_transport_t *t;
    if (!out)
    {
        return VM_INVALID;
    }
    *out = NULL;
    t = calloc(1, sizeof(*t));
    if (!t)
    {
        return VM_NOMEM;
    }
    t->base.ops = &ops;
    t->base.context = t;
    *out = t;
    return VM_OK;
}
vm_status_t vm_loopback_transport_destroy(vm_loopback_transport_t *t)
{
    if (!t)
    {
        return VM_OK;
    }
    free(t);
    return VM_OK;
}
vm_transport_t *vm_loopback_transport_base(vm_loopback_transport_t *t)
{
    return t ? &t->base : NULL;
}
