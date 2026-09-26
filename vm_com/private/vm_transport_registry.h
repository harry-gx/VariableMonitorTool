#ifndef VM_TRANSPORT_REGISTRY_H
#define VM_TRANSPORT_REGISTRY_H
#include "vm_transport.h"
#include "vm_list.h"
VM_BEGIN
typedef struct vm_transport_registry vm_transport_registry_t;
typedef struct vm_transport_node
{
    const char *id;
    const char *display_name;
    const vm_transport_ops_t *ops;
    void *context;
    list_head_t link;
    vm_transport_registry_t *owner;
    size_t references;
} vm_transport_node_t;
struct vm_transport_registry
{
    list_head_t head;
};
void vm_transport_registry_init(vm_transport_registry_t *registry);
vm_status_t vm_transport_registry_register(vm_transport_registry_t *registry,
                                           vm_transport_node_t *node);
vm_status_t vm_transport_registry_unregister(vm_transport_registry_t *registry,
                                             vm_transport_node_t *node);
vm_transport_node_t *
vm_transport_registry_find(vm_transport_registry_t *registry, const char *id);
size_t vm_transport_registry_count(const vm_transport_registry_t *registry);
vm_status_t vm_transport_registry_at(const vm_transport_registry_t *registry,
                                     size_t index,
                                     vm_transport_node_t **out);
vm_status_t vm_transport_registry_acquire(vm_transport_registry_t *registry,
                                          const char *id,
                                          vm_transport_node_t **out);
vm_status_t vm_transport_registry_release(vm_transport_registry_t *registry,
                                          vm_transport_node_t *node);
VM_END
#endif
