#include "vm_protocol_registry.h"
#include <stdio.h>
#define C(x)                                                                   \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            fprintf(stderr, "registry line %d: %s\n", __LINE__, #x);           \
            return 1;                                                          \
        }                                                                      \
    } while (0)
static vm_status_t reset(vm_protocol_t *p)
{
    (void)p;
    return VM_OK;
}
static vm_status_t receive(vm_protocol_t *p, const vm_transport_packet_t *f)
{
    (void)p;
    (void)f;
    return VM_OK;
}
int main(void)
{
    const vm_protocol_ops_t ops = {reset, receive, NULL, NULL};
    vm_protocol_registry_t a;
    vm_protocol_registry_t b;
    vm_protocol_node_t x = {0};
    vm_protocol_node_t y = {0};
    vm_protocol_node_t duplicate = {0};
    vm_protocol_node_t *out = NULL;
    x.id = "x";
    x.display_name = "X";
    x.ops = &ops;
    y.id = "y";
    y.display_name = "Y";
    y.ops = &ops;
    duplicate.id = "x";
    duplicate.display_name = "Duplicate";
    duplicate.ops = &ops;
    vm_protocol_registry_init(&a);
    vm_protocol_registry_init(&b);
    C(vm_protocol_registry_unregister(&a, &x) == VM_NOT_FOUND);
    C(vm_protocol_registry_register(&a, &x) == VM_OK);
    C(vm_protocol_registry_register(&a, &x) == VM_DUPLICATE);
    C(vm_protocol_registry_register(&b, &x) == VM_BUSY);
    C(vm_protocol_registry_register(&a, &duplicate) == VM_DUPLICATE);
    C(vm_protocol_registry_register(&a, &y) == VM_OK);
    C(vm_protocol_registry_count(&a) == 2 &&
      vm_protocol_registry_count(&b) == 0);
    C(vm_protocol_registry_at(&a, 0, &out) == VM_OK && out == &x);
    C(vm_protocol_registry_at(&a, 1, &out) == VM_OK && out == &y);
    C(vm_protocol_registry_at(&a, 2, &out) == VM_NOT_FOUND && out == &y);
    C(vm_protocol_registry_acquire(&a, "x", &out) == VM_OK && out == &x);
    C(vm_protocol_registry_acquire(&a, "x", &out) == VM_OK);
    C(vm_protocol_registry_unregister(&a, &x) == VM_BUSY);
    C(vm_protocol_registry_release(&b, &x) == VM_NOT_FOUND);
    C(vm_protocol_registry_release(&a, &x) == VM_OK);
    C(vm_protocol_registry_unregister(&a, &x) == VM_BUSY);
    C(vm_protocol_registry_release(&a, &x) == VM_OK);
    C(vm_protocol_registry_release(&a, &x) == VM_INVALID);
    C(vm_protocol_registry_unregister(&a, &x) == VM_OK);
    C(vm_protocol_registry_unregister(&a, &x) == VM_NOT_FOUND);
    C(vm_protocol_registry_count(&a) == 1);
    C(vm_protocol_registry_register(&b, &x) == VM_OK);
    C(vm_protocol_registry_find(&a, "x") == NULL &&
      vm_protocol_registry_find(&b, "x") == &x);
    C(vm_protocol_registry_unregister(&b, &x) == VM_OK);
    C(vm_protocol_registry_unregister(&a, &y) == VM_OK);
    x.id = "";
    C(vm_protocol_registry_register(&a, &x) == VM_INVALID);
    C(vm_protocol_registry_acquire(&a, "missing", &out) == VM_NOT_FOUND &&
      out == &x);
    C(vm_protocol_registry_at(NULL, 0, &out) == VM_INVALID);
    puts(
        "Protocol registry ownership, enumeration and reference tests passed.");
    return 0;
}
