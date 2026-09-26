#ifndef VM_SERIAL_TRANSPORT_H
#define VM_SERIAL_TRANSPORT_H
#include "vm_transport.h"
VM_BEGIN
typedef struct
{
    const char *device;
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t flow_control;
} vm_serial_config_t;
enum
{
    VM_SERIAL_PARITY_NONE = 0,
    VM_SERIAL_PARITY_ODD = 1,
    VM_SERIAL_PARITY_EVEN = 2
};
enum
{
    VM_SERIAL_FLOW_NONE = 0,
    VM_SERIAL_FLOW_RTS_CTS = 1,
    VM_SERIAL_FLOW_XON_XOFF = 2
};
static inline vm_status_t vm_serial_config_validate(const vm_serial_config_t *c)
{
    if (!c || !c->device || !*c->device || !c->baudrate)
    {
        return VM_INVALID;
    }
    if (c->data_bits < 5 || c->data_bits > 8 ||
        (c->stop_bits != 1 && c->stop_bits != 2))
    {
        return VM_FORMAT;
    }
    if (c->parity > VM_SERIAL_PARITY_EVEN ||
        c->flow_control > VM_SERIAL_FLOW_XON_XOFF)
    {
        return VM_FORMAT;
    }
    return VM_OK;
}
typedef struct vm_serial_transport vm_serial_transport_t;
vm_status_t vm_serial_transport_create(const vm_serial_config_t *config,
                                       vm_serial_transport_t **out);
vm_status_t vm_serial_transport_destroy(vm_serial_transport_t *transport);
vm_transport_t *vm_serial_transport_base(vm_serial_transport_t *transport);
vm_status_t vm_serial_transport_receive_once(vm_serial_transport_t *transport,
                                             uint32_t wait_ms);
VM_END
#endif
