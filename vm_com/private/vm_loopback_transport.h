#ifndef VM_LOOPBACK_TRANSPORT_H
#define VM_LOOPBACK_TRANSPORT_H
#include "vm_transport.h"
VM_BEGIN
typedef struct vm_loopback_transport vm_loopback_transport_t;
vm_status_t vm_loopback_transport_create(vm_loopback_transport_t **out);
vm_status_t vm_loopback_transport_destroy(vm_loopback_transport_t *transport);
vm_transport_t *vm_loopback_transport_base(vm_loopback_transport_t *transport);
VM_END
#endif
