#ifndef VM_PROTOCOL_H
#define VM_PROTOCOL_H
#include "vm_transport.h"
VM_BEGIN
typedef struct vm_protocol vm_protocol_t;
typedef struct
{
    uint8_t *data;
    size_t size;
    size_t capacity;
    uint32_t channel;
} vm_protocol_packet_t;
typedef struct
{
    vm_status_t (*reset)(vm_protocol_t *protocol);
    vm_status_t (*on_transport_packet)(vm_protocol_t *protocol,
                                       const vm_transport_packet_t *packet);
    vm_status_t (*read_memory)(vm_protocol_t *protocol,
                               uint64_t address,
                               uint8_t *data,
                               size_t size);
    vm_status_t (*write_memory)(vm_protocol_t *protocol,
                                uint64_t address,
                                const uint8_t *data,
                                size_t size);
} vm_protocol_ops_t;
struct vm_protocol
{
    const vm_protocol_ops_t *ops;
    vm_transport_t *transport;
    void *context;
};
static inline vm_status_t vm_protocol_send(vm_protocol_t *p,
                                           const uint8_t *data,
                                           size_t size,
                                           uint32_t channel)
{
    if (!p || (!data && size) || !p->transport || !p->transport->ops ||
        !p->transport->ops->send)
    {
        return VM_INVALID;
    }
    return p->transport->ops->send(p->transport, data, size, channel);
}
VM_END
#endif
