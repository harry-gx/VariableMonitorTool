#include "vm_can_transport.h"
#include <stdlib.h>
struct vm_can_transport
{
    vm_transport_t base;
    vm_can_config_t config;
    vm_transport_rx_fn rx;
    void *rx_context;
};
static vm_status_t can_open(vm_transport_t *b)
{
    (void)b;
    return VM_UNSUPPORTED;
}
static vm_status_t can_close(vm_transport_t *b)
{
    if (b)
    {
        b->opened = 0;
    }
    return VM_OK;
}
static vm_status_t
can_send(vm_transport_t *b, const uint8_t *d, size_t n, uint32_t c)
{
    (void)b;
    (void)d;
    (void)n;
    (void)c;
    return VM_UNSUPPORTED;
}
static vm_status_t can_rx(vm_transport_t *b, vm_transport_rx_fn f, void *c)
{
    struct vm_can_transport *t = (struct vm_can_transport *)b;
    if (!t)
    {
        return VM_INVALID;
    }
    t->rx = f;
    t->rx_context = c;
    return VM_OK;
}
static const vm_transport_ops_t ops = {can_open, can_close, can_send, can_rx};
vm_status_t vm_can_transport_create(const vm_can_config_t *c,
                                    vm_can_transport_t **out)
{
    vm_can_transport_t *t;
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    s = vm_can_config_validate(c);
    if (s != VM_OK)
    {
        return s;
    }
    *out = NULL;
    t = calloc(1, sizeof(*t));
    if (!t)
    {
        return VM_NOMEM;
    }
    t->config = *c;
    t->base.ops = &ops;
    t->base.context = t;
    *out = t;
    return VM_OK;
}
vm_status_t vm_can_transport_destroy(vm_can_transport_t *t)
{
    if (!t)
    {
        return VM_OK;
    }
    can_close(&t->base);
    free(t);
    return VM_OK;
}
vm_transport_t *vm_can_transport_base(vm_can_transport_t *t)
{
    return t ? t ? &t->base : NULL : NULL;
}
vm_status_t vm_can_transport_receive_once(vm_can_transport_t *t,
                                          uint32_t wait_ms)
{
    (void)wait_ms;
    if (!t)
    {
        return VM_INVALID;
    }
    return VM_UNSUPPORTED;
}
