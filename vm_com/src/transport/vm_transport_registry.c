#include "vm_transport_registry.h"
#include <string.h>
void vm_transport_registry_init(vm_transport_registry_t *r)
{
    if (r)
    {
        INIT_LIST_HEAD(&r->head);
    }
}
vm_transport_node_t *vm_transport_registry_find(vm_transport_registry_t *r,
                                                const char *id)
{
    list_head_t *p;
    if (!r || !id)
    {
        return NULL;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        vm_transport_node_t *n = list_entry(p, vm_transport_node_t, link);
        if (!strcmp(n->id, id))
        {
            return n;
        }
    }
    return NULL;
}
vm_status_t vm_transport_registry_register(vm_transport_registry_t *r,
                                           vm_transport_node_t *n)
{
    if (!r || !n || !n->id || !*n->id || !n->display_name ||
        !*n->display_name || !n->ops || !n->ops->open || !n->ops->close ||
        !n->ops->send || !n->ops->set_rx)
    {
        return VM_INVALID;
    }
    if (n->owner)
    {
        return n->owner == r ? VM_DUPLICATE : VM_BUSY;
    }
    if (vm_transport_registry_find(r, n->id))
    {
        return VM_DUPLICATE;
    }
    INIT_LIST_HEAD(&n->link);
    LIST_ADD_TAIL(&n->link, &r->head);
    n->owner = r;
    n->references = 0;
    return VM_OK;
}
vm_status_t vm_transport_registry_unregister(vm_transport_registry_t *r,
                                             vm_transport_node_t *n)
{
    list_head_t *p;
    if (!r || !n)
    {
        return VM_INVALID;
    }
    if (n->owner != r)
    {
        return VM_NOT_FOUND;
    }
    if (n->references)
    {
        return VM_BUSY;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (list_entry(p, vm_transport_node_t, link) == n)
        {
            LIST_DEL(&n->link);
            INIT_LIST_HEAD(&n->link);
            n->owner = NULL;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
size_t vm_transport_registry_count(const vm_transport_registry_t *r)
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
vm_status_t vm_transport_registry_at(const vm_transport_registry_t *r,
                                     size_t i,
                                     vm_transport_node_t **out)
{
    list_head_t *p;
    if (!r || !out)
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (i-- == 0)
        {
            *out = list_entry(p, vm_transport_node_t, link);
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
vm_status_t vm_transport_registry_acquire(vm_transport_registry_t *r,
                                          const char *id,
                                          vm_transport_node_t **out)
{
    vm_transport_node_t *n;
    if (!r || !id || !out)
    {
        return VM_INVALID;
    }
    n = vm_transport_registry_find(r, id);
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (n->references == SIZE_MAX)
    {
        return VM_BUSY;
    }
    ++n->references;
    *out = n;
    return VM_OK;
}
vm_status_t vm_transport_registry_release(vm_transport_registry_t *r,
                                          vm_transport_node_t *n)
{
    if (!r || !n)
    {
        return VM_INVALID;
    }
    if (n->owner != r)
    {
        return VM_NOT_FOUND;
    }
    if (!n->references)
    {
        return VM_INVALID;
    }
    --n->references;
    return VM_OK;
}
