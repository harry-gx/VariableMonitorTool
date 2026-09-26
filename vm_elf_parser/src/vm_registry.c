#include "vm_registry.h"
#include "vm_list.h"
#include <stdlib.h>
#include <string.h>
typedef struct
{
    list_head_t link;
    vm_adapter_info_t info;
    const void *ops;
    size_t refs;
} node;
struct vm_registry
{
    list_head_t head;
    size_t count;
};
static char *copy(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p)
    {
        memcpy(p, s, n);
    }
    return p;
}
static node *find(vm_registry_t *r, const char *id)
{
    list_head_t *p;
    if (!r || !id)
    {
        return NULL;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        node *n = list_entry(p, node, link);
        if (!strcmp(n->info.id, id))
        {
            return n;
        }
    }
    return NULL;
}
vm_status_t vm_registry_create(vm_registry_t **out)
{
    vm_registry_t *r;
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
vm_status_t
vm_registry_add(vm_registry_t *r, const vm_adapter_info_t *i, const void *ops)
{
    node *n;
    if (!r || !i || !i->id || !*i->id || !i->name || !ops)
    {
        return VM_INVALID;
    }
    if (find(r, i->id))
    {
        return VM_DUPLICATE;
    }
    n = calloc(1, sizeof(*n));
    if (!n)
    {
        return VM_NOMEM;
    }
    n->info = *i;
    n->info.id = copy(i->id);
    n->info.name = copy(i->name);
    n->ops = ops;
    if (!n->info.id || !n->info.name)
    {
        free((void *)n->info.id);
        free((void *)n->info.name);
        free(n);
        return VM_NOMEM;
    }
    INIT_LIST_HEAD(&n->link);
    LIST_ADD_TAIL(&n->link, &r->head);
    r->count++;
    return VM_OK;
}
vm_status_t vm_registry_remove(vm_registry_t *r, const char *id)
{
    node *n = find(r, id);
    if (!r || !id)
    {
        return VM_INVALID;
    }
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (n->refs)
    {
        return VM_BUSY;
    }
    LIST_DEL(&n->link);
    free((void *)n->info.id);
    free((void *)n->info.name);
    free(n);
    r->count--;
    return VM_OK;
}
vm_status_t vm_registry_destroy(vm_registry_t *r)
{
    list_head_t *p;
    if (!r)
    {
        return VM_OK;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (list_entry(p, node, link)->refs)
        {
            return VM_BUSY;
        }
    }
    while (!LIST_EMPTY(&r->head))
    {
        node *n = list_entry(r->head.next, node, link);
        vm_registry_remove(r, n->info.id);
    }
    free(r);
    return VM_OK;
}
size_t vm_registry_count(const vm_registry_t *r)
{
    return r ? r->count : 0;
}
vm_status_t
vm_registry_at(const vm_registry_t *r, size_t i, vm_adapter_info_t *out)
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
            *out = list_entry(p, node, link)->info;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
vm_status_t
vm_registry_acquire(vm_registry_t *r, const char *id, const void **ops)
{
    node *n = find(r, id);
    if (!r || !id || !ops)
    {
        return VM_INVALID;
    }
    *ops = NULL;
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (n->refs == SIZE_MAX)
    {
        return VM_BUSY;
    }
    n->refs++;
    *ops = n->ops;
    return VM_OK;
}
vm_status_t vm_registry_release(vm_registry_t *r, const char *id)
{
    node *n = find(r, id);
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (!n->refs)
    {
        return VM_INVALID;
    }
    n->refs--;
    return VM_OK;
}
