#ifndef VM_CAN_TRANSPORT_H
#define VM_CAN_TRANSPORT_H
#include "vm_transport.h"
VM_BEGIN
typedef enum
{
    VM_CAN_VENDOR_ECANVCI = 1,
    VM_CAN_VENDOR_ZLG = 2
} vm_can_vendor_t;
typedef struct
{
    vm_can_vendor_t vendor;
    uint32_t device_type;
    uint32_t device_index;
    uint32_t channel_index;
    uint32_t nominal_bitrate;
    uint32_t data_bitrate;
    uint8_t fd;
    uint8_t brs;
    uint8_t listen_only;
} vm_can_config_t;
static inline vm_status_t vm_can_config_validate(const vm_can_config_t *c)
{
    if (!c ||
        (c->vendor != VM_CAN_VENDOR_ECANVCI &&
         c->vendor != VM_CAN_VENDOR_ZLG) ||
        !c->nominal_bitrate)
    {
        return VM_INVALID;
    }
    if (c->fd && !c->data_bitrate)
    {
        return VM_INVALID;
    }
    if (!c->fd && (c->brs || c->data_bitrate))
    {
        return VM_FORMAT;
    }
    if (c->fd > 1 || c->brs > 1 || c->listen_only > 1)
    {
        return VM_FORMAT;
    }
    return VM_OK;
}
typedef struct
{
    uint32_t id;
    uint8_t extended;
    uint8_t remote;
    uint8_t fd;
    uint8_t brs;
    uint8_t data_length;
    uint8_t data[64];
    uint64_t timestamp;
} vm_can_frame_t;
static inline vm_status_t vm_can_frame_validate(const vm_can_frame_t *f)
{
    if (!f)
    {
        return VM_INVALID;
    }
    if (f->extended ? f->id > 0x1fffffffu : f->id > 0x7ffu)
    {
        return VM_FORMAT;
    }
    if (f->remote && f->data_length)
    {
        return VM_FORMAT;
    }
    if (f->data_length > (f->fd ? 64u : 8u))
    {
        return VM_FORMAT;
    }
    if (!f->fd && f->brs)
    {
        return VM_FORMAT;
    }
    return VM_OK;
}
typedef struct vm_can_transport vm_can_transport_t;
vm_status_t vm_can_transport_create(const vm_can_config_t *config,
                                    vm_can_transport_t **out);
vm_status_t vm_can_transport_destroy(vm_can_transport_t *transport);
vm_transport_t *vm_can_transport_base(vm_can_transport_t *transport);
vm_status_t vm_can_transport_receive_once(vm_can_transport_t *transport,
                                          uint32_t wait_ms);
VM_END
#endif
