#include "vm_loopback_transport.h"
#include "vm_router.h"
#include <stdio.h>
#include <string.h>
#define C(x)                                                                   \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            fprintf(stderr, "loopback line %d: %s\n", __LINE__, #x);           \
            return 1;                                                          \
        }                                                                      \
    } while (0)
typedef struct
{
    vm_router_t *router;
    unsigned rx;
} ctx_t;
static vm_status_t protocol_rx(void *p, const vm_frame_t *f)
{
    ctx_t *c = p;
    if (f->size == 2 && f->data[0] == 0xa5)
    {
        c->rx++;
    }
    return VM_OK;
}
static vm_status_t transport_send(void *p, const vm_frame_t *f)
{
    vm_transport_t *t = p;
    return vm_transport_send(t, f->data, f->size, f->channel);
}
static vm_status_t loop_rx(void *p, const vm_transport_packet_t *f)
{
    ctx_t *c = p;
    vm_frame_t frame = {1, f->channel, 0, f->data, f->size};
    return vm_router_receive(c->router, &frame);
}
int main(void)
{
    vm_loopback_transport_t *loop = NULL;
    vm_transport_t *base;
    vm_router_t *r = NULL;
    ctx_t c = {0};
    vm_route_t route = {7, 1, 3, 0, 0, protocol_rx, transport_send, &c, NULL};
    uint8_t d[2] = {0xa5, 1};
    C(vm_loopback_transport_create(&loop) == VM_OK);
    base = vm_loopback_transport_base(loop);
    C(base);
    C(vm_transport_open(base) == VM_OK);
    route.transport_context = base;
    C(base->ops->set_rx(base, loop_rx, &c) == VM_OK);
    C(vm_router_create(&r) == VM_OK);
    c.router = r;
    C(vm_router_bind(r, &route) == VM_OK);
    C(vm_router_send(r, 7, d, 2) == VM_OK);
    C(c.rx == 1);
    C(vm_router_destroy(r) == VM_OK);
    C(vm_transport_close(base) == VM_OK);
    C(vm_loopback_transport_destroy(loop) == VM_OK);
    puts("Loopback transport and router path passed.");
    return 0;
}
