#include "vm_custom_protocol.h"

#include <stdio.h>
#include <string.h>

#define C(x)                                                                   \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            printf("fail %s:%d\n", __FILE__, __LINE__);                        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static vm_status_t got(void *context, const vm_custom_message_t *message)
{
    *(vm_custom_message_t *)context = *message;
    return VM_OK;
}

int main(void)
{
    uint8_t data[64];
    uint8_t part[64];
    size_t written;
    vm_custom_message_t write_message = {
        1u, VM_CUSTOM_WRITE, 7u, 0x12345678u, (const uint8_t *)"abc", 3u};
    vm_custom_message_t out = {0};
    vm_custom_parser_t *parser = NULL;

    C(vm_custom_make_read(3u, 0x2000005Cu, 4u, data, sizeof(data), &written) ==
      VM_OK);
    C(written == 13u);
    C(vm_custom_decode(data, written, &out) == VM_OK);
    C(out.command == VM_CUSTOM_READ);
    C(out.sequence == 3u);
    C(out.address == 0x2000005Cu);
    C(out.length == 4u);
    C(out.payload == NULL);

    C(vm_custom_encode(&write_message, data, sizeof(data), &written) == VM_OK &&
      written == 16u);
    C(vm_custom_decode(data, written, &out) == VM_OK);
    C(out.address == write_message.address && out.length == 3u &&
      !memcmp(out.payload, "abc", 3u));

    data[written - 1u] ^= 1u;
    C(vm_custom_decode(data, written, &out) == VM_FORMAT);
    data[written - 1u] ^= 1u;

    C(vm_custom_parser_create(&parser) == VM_OK);
    memcpy(part, data, 5u);
    C(vm_custom_parser_feed(parser, part, 5u, got, &out) == VM_OK);
    C(vm_custom_parser_feed(parser, data + 5u, written - 5u, got, &out) ==
      VM_OK);
    C(out.sequence == 7u);
    vm_custom_parser_destroy(parser);

    puts("Custom protocol read/write encode, CRC and streaming tests passed.");
    return 0;
}
