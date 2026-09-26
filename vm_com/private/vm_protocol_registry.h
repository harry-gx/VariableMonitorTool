#ifndef VM_PROTOCOL_REGISTRY_H
#define VM_PROTOCOL_REGISTRY_H
#include "vm_protocol.h"
#include "vm_list.h"
VM_BEGIN
typedef struct vm_protocol_registry vm_protocol_registry_t;
/* Zero-initialize nodes; keep descriptors and storage alive and immutable while registered. */
typedef struct vm_protocol_node
{
    const char *id;
    const char *display_name;
    const vm_protocol_ops_t *ops;
    void *context;
    list_head_t link;
    vm_protocol_registry_t *owner;
    size_t references;
} vm_protocol_node_t;
struct vm_protocol_registry
{
    list_head_t head;
};
void vm_protocol_registry_init(vm_protocol_registry_t *registry);
vm_status_t vm_protocol_registry_register(vm_protocol_registry_t *registry,
                                          vm_protocol_node_t *node);
vm_status_t vm_protocol_registry_unregister(vm_protocol_registry_t *registry,
                                            vm_protocol_node_t *node);
vm_protocol_node_t *vm_protocol_registry_find(vm_protocol_registry_t *registry,
                                              const char *id);
size_t vm_protocol_registry_count(const vm_protocol_registry_t *registry);
/* Single service thread. Acquire before retaining a node for an active session. */
vm_status_t vm_protocol_registry_acquire(vm_protocol_registry_t *registry,
                                         const char *id,
                                         vm_protocol_node_t **out);
vm_status_t vm_protocol_registry_release(vm_protocol_registry_t *registry,
                                         vm_protocol_node_t *node);
vm_status_t vm_protocol_registry_at(const vm_protocol_registry_t *registry,
                                    size_t index,
                                    vm_protocol_node_t **out);
VM_END
#endif
