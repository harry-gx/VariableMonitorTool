#ifndef VM_ROUTER_H
#define VM_ROUTER_H
#include "vm_status.h"
VM_BEGIN
typedef struct vm_router vm_router_t;
/* Transport identity is a session/connection token, not a driver type.
   Frame and payload borrowed only until callback returns. */
typedef struct
{
    uint64_t connection;
    uint32_t channel;
    uint32_t address;
    const uint8_t *data;
    size_t size;
} vm_frame_t;
typedef vm_status_t (*vm_frame_fn)(void *context, const vm_frame_t *frame);
typedef struct
{
    uint64_t route_id;
    uint64_t connection;
    uint32_t channel;
    uint32_t rx_address;
    uint32_t tx_address;
    vm_frame_fn protocol_receive;
    vm_frame_fn transport_send;
    void *protocol_context;
    void *transport_context;
} vm_route_t;
/* All calls on one service thread. Callbacks may send, but not mutate routes. */
vm_status_t vm_router_create(vm_router_t **out);
vm_status_t vm_router_destroy(vm_router_t *r);
vm_status_t vm_router_bind(vm_router_t *r, const vm_route_t *route);
vm_status_t vm_router_unbind(vm_router_t *r, uint64_t route_id);
vm_status_t vm_router_receive(vm_router_t *r, const vm_frame_t *frame);
vm_status_t vm_router_send(vm_router_t *r,
                           uint64_t route_id,
                           const uint8_t *data,
                           size_t size);
size_t vm_router_route_count(const vm_router_t *r);
vm_status_t
vm_router_route_at(const vm_router_t *r, size_t index, vm_route_t *out);
VM_END
#endif
