#ifndef VM_TRANSPORT_H
#define VM_TRANSPORT_H
#include "vm_status.h"
VM_BEGIN
typedef struct vm_transport vm_transport_t;
typedef struct
{
    const uint8_t *data;
    size_t size;
    uint32_t channel;
    uint64_t timestamp;
} vm_transport_packet_t;
typedef vm_status_t (*vm_transport_rx_fn)(void *context,
                                          const vm_transport_packet_t *packet);
typedef struct
{
    vm_status_t (*open)(vm_transport_t *transport);
    vm_status_t (*close)(vm_transport_t *transport);
    vm_status_t (*send)(vm_transport_t *transport,
                        const uint8_t *data,
                        size_t size,
                        uint32_t channel);
    vm_status_t (*set_rx)(vm_transport_t *transport,
                          vm_transport_rx_fn callback,
                          void *context);
} vm_transport_ops_t;
struct vm_transport
{
    const vm_transport_ops_t *ops;
    void *context;
    uint8_t opened;
};
static inline vm_status_t vm_transport_open(vm_transport_t *t)
{
    return t && t->ops && t->ops->open ? t->ops->open(t) : VM_INVALID;
}
static inline vm_status_t vm_transport_close(vm_transport_t *t)
{
    return t && t->ops && t->ops->close ? t->ops->close(t) : VM_INVALID;
}
static inline vm_status_t
vm_transport_send(vm_transport_t *t, const uint8_t *d, size_t n, uint32_t c)
{
    return t && t->ops && t->ops->send ? t->ops->send(t, d, n, c) : VM_INVALID;
}
VM_END
#endif
