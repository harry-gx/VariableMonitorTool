#ifndef VM_CUSTOM_PROTOCOL_H
#define VM_CUSTOM_PROTOCOL_H
#include "vm_protocol.h"
VM_BEGIN
#define VM_CUSTOM_SOF0 0xAAu
#define VM_CUSTOM_SOF1 0x55u
enum
{
    VM_CUSTOM_READ = 1,
    VM_CUSTOM_WRITE = 2,
    VM_CUSTOM_READ_RESPONSE = 0x81,
    VM_CUSTOM_WRITE_RESPONSE = 0x82,
    VM_CUSTOM_ERROR = 0xE0
};
typedef struct
{
    uint8_t version;
    uint8_t command;
    uint8_t sequence;
    uint32_t address;
    const uint8_t *payload;
    uint16_t length;
} vm_custom_message_t;
typedef struct vm_custom_parser vm_custom_parser_t;
vm_status_t vm_custom_make_read(uint8_t sequence,
                                uint32_t address,
                                uint16_t length,
                                uint8_t *out,
                                size_t capacity,
                                size_t *written);
vm_status_t vm_custom_make_write(uint8_t sequence,
                                 uint32_t address,
                                 const uint8_t *data,
                                 uint16_t length,
                                 uint8_t *out,
                                 size_t capacity,
                                 size_t *written);
uint16_t vm_custom_crc16(const uint8_t *data, size_t size);
size_t vm_custom_encoded_size(const vm_custom_message_t *message);
vm_status_t vm_custom_encode(const vm_custom_message_t *message,
                             uint8_t *out,
                             size_t capacity,
                             size_t *written);
vm_status_t vm_custom_decode(const uint8_t *data,
                             size_t size,
                             vm_custom_message_t *message);
vm_status_t vm_custom_parser_create(vm_custom_parser_t **out);
void vm_custom_parser_destroy(vm_custom_parser_t *parser);
vm_status_t vm_custom_parser_feed(
    vm_custom_parser_t *parser,
    const uint8_t *data,
    size_t size,
    vm_status_t (*callback)(void *, const vm_custom_message_t *),
    void *context);
VM_END
#endif
