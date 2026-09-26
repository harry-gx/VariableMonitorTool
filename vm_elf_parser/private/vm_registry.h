#ifndef VM_REGISTRY_H
#define VM_REGISTRY_H
#include "vm_status.h"
VM_BEGIN
typedef struct vm_registry vm_registry_t;
typedef struct
{
    const char *id;
    const char *name;
    uint32_t capabilities;
} vm_adapter_info_t;
/* Registry owns copied metadata, borrows ops. Single service-thread use.
   acquired ops must be released before unregister/destroy. */
vm_status_t vm_registry_create(vm_registry_t **out);
vm_status_t vm_registry_destroy(vm_registry_t *r);
vm_status_t vm_registry_add(vm_registry_t *r,
                            const vm_adapter_info_t *info,
                            const void *ops);
vm_status_t vm_registry_remove(vm_registry_t *r, const char *id);
size_t vm_registry_count(const vm_registry_t *r);
vm_status_t
vm_registry_at(const vm_registry_t *r, size_t index, vm_adapter_info_t *out);
vm_status_t
vm_registry_acquire(vm_registry_t *r, const char *id, const void **ops);
vm_status_t vm_registry_release(vm_registry_t *r, const char *id);
VM_END
#endif
