#include "vm_router.h"
#include "vm_list.h"
#include <stdlib.h>
typedef struct
{
    list_head_t link;
    vm_route_t route;
} binding;
struct vm_router
{
    list_head_t head;
    size_t dispatching;
};
vm_status_t vm_router_create(vm_router_t **out)
{
    vm_router_t *r;
    if (!out)
    {
        return VM_INVALID;
    }
    *out = NULL;
    r = calloc(1, sizeof(*r));
    if (!r)
    {
        return VM_NOMEM;
    }
    INIT_LIST_HEAD(&r->head);
    *out = r;
    return VM_OK;
}
vm_status_t vm_router_bind(vm_router_t *r, const vm_route_t *v)
{
    list_head_t *p;
    binding *b;
    if (!r || !v || !v->protocol_receive || !v->transport_send)
    {
        return VM_INVALID;
    }
    if (r->dispatching)
    {
        return VM_BUSY;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        const vm_route_t *x = &list_entry(p, binding, link)->route;
        if (x->route_id == v->route_id)
        {
            return VM_DUPLICATE;
        }
        if (x->connection == v->connection && x->channel == v->channel &&
            x->rx_address == v->rx_address)
        {
            return VM_AMBIGUOUS;
        }
    }
    b = malloc(sizeof(*b));
    if (!b)
    {
        return VM_NOMEM;
    }
    b->route = *v;
    INIT_LIST_HEAD(&b->link);
    LIST_ADD_TAIL(&b->link, &r->head);
    return VM_OK;
}
vm_status_t vm_router_unbind(vm_router_t *r, uint64_t id)
{
    list_head_t *p;
    if (!r)
    {
        return VM_INVALID;
    }
    if (r->dispatching)
    {
        return VM_BUSY;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        binding *b = list_entry(p, binding, link);
        if (b->route.route_id == id)
        {
            LIST_DEL(p);
            free(b);
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
vm_status_t vm_router_destroy(vm_router_t *r)
{
    if (!r)
    {
        return VM_OK;
    }
    if (r->dispatching)
    {
        return VM_BUSY;
    }
    while (!LIST_EMPTY(&r->head))
    {
        binding *b = list_entry(r->head.next, binding, link);
        LIST_DEL(&b->link);
        free(b);
    }
    free(r);
    return VM_OK;
}
vm_status_t vm_router_receive(vm_router_t *r, const vm_frame_t *f)
{
    list_head_t *p;
    if (!r || !f || (!f->data && f->size))
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        vm_route_t *v = &list_entry(p, binding, link)->route;
        if (v->connection == f->connection && v->channel == f->channel &&
            v->rx_address == f->address)
        {
            vm_status_t s;
            r->dispatching++;
            s = v->protocol_receive(v->protocol_context, f);
            r->dispatching--;
            return s;
        }
    }
    return VM_NOT_FOUND;
}
vm_status_t
vm_router_send(vm_router_t *r, uint64_t id, const uint8_t *data, size_t size)
{
    list_head_t *p;
    if (!r || (!data && size))
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        vm_route_t *v = &list_entry(p, binding, link)->route;
        if (v->route_id == id)
        {
            vm_frame_t f;
            vm_status_t s;
            f.connection = v->connection;
            f.channel = v->channel;
            f.address = v->tx_address;
            f.data = data;
            f.size = size;
            r->dispatching++;
            s = v->transport_send(v->transport_context, &f);
            r->dispatching--;
            return s;
        }
    }
    return VM_NOT_FOUND;
}
size_t vm_router_route_count(const vm_router_t *r)
{
    size_t n = 0;
    list_head_t *p;
    if (!r)
    {
        return 0;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        ++n;
    }
    return n;
}
vm_status_t
vm_router_route_at(const vm_router_t *r, size_t index, vm_route_t *out)
{
    list_head_t *p;
    if (!r || !out)
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (index-- == 0)
        {
            *out = list_entry(p, binding, link)->route;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
