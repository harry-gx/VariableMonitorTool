#include "vm_custom_protocol.h"

#include <stdlib.h>
#include <string.h>

struct vm_custom_parser
{
    uint8_t *buffer;
    size_t size;
    size_t capacity;
};

static void put16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

static uint16_t get16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t get32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static size_t wire_payload_size(uint8_t command, uint16_t length)
{
    return command == VM_CUSTOM_READ ? 0u : (size_t)length;
}

uint16_t vm_custom_crc16(const uint8_t *data, size_t size)
{
    uint16_t crc = 0xFFFFu;
    size_t index;
    unsigned bit;

    for (index = 0; index < size; ++index)
    {
        crc ^= data[index];
        for (bit = 0; bit < 8u; ++bit)
        {
            crc = (uint16_t)((crc >> 1) ^ ((crc & 1u) ? 0xA001u : 0u));
        }
    }

    return crc;
}

vm_status_t vm_custom_make_read(uint8_t sequence,
                                uint32_t address,
                                uint16_t length,
                                uint8_t *out,
                                size_t capacity,
                                size_t *written)
{
    vm_custom_message_t message = {
        1u, VM_CUSTOM_READ, sequence, address, NULL, length};
    return vm_custom_encode(&message, out, capacity, written);
}

vm_status_t vm_custom_make_write(uint8_t sequence,
                                 uint32_t address,
                                 const uint8_t *data,
                                 uint16_t length,
                                 uint8_t *out,
                                 size_t capacity,
                                 size_t *written)
{
    vm_custom_message_t message = {
        1u, VM_CUSTOM_WRITE, sequence, address, data, length};
    return vm_custom_encode(&message, out, capacity, written);
}

size_t vm_custom_encoded_size(const vm_custom_message_t *message)
{
    if (!message)
    {
        return 0u;
    }

    return wire_payload_size(message->command, message->length) + 13u;
}

vm_status_t vm_custom_encode(const vm_custom_message_t *message,
                             uint8_t *out,
                             size_t capacity,
                             size_t *written)
{
    size_t payload_size;
    size_t total;
    uint16_t crc;

    if (!message || !out || !written || message->length > 1024u)
    {
        return VM_INVALID;
    }

    payload_size = wire_payload_size(message->command, message->length);
    if (payload_size > 0u && !message->payload)
    {
        return VM_INVALID;
    }

    total = payload_size + 13u;
    if (capacity < total)
    {
        return VM_NOMEM;
    }

    out[0] = VM_CUSTOM_SOF0;
    out[1] = VM_CUSTOM_SOF1;
    out[2] = message->version;
    out[3] = message->command;
    out[4] = message->sequence;
    put32(out + 5, message->address);
    put16(out + 9, message->length);

    if (payload_size > 0u)
    {
        memcpy(out + 11, message->payload, payload_size);
    }

    crc = vm_custom_crc16(out + 2, 9u + payload_size);
    put16(out + 11 + payload_size, crc);
    *written = total;

    return VM_OK;
}

vm_status_t
vm_custom_decode(const uint8_t *data, size_t size, vm_custom_message_t *message)
{
    uint8_t command;
    uint16_t length;
    size_t payload_size;
    uint16_t crc;

    if (!data || !message || size < 13u || data[0] != VM_CUSTOM_SOF0 ||
        data[1] != VM_CUSTOM_SOF1)
    {
        return VM_FORMAT;
    }

    command = data[3];
    length = get16(data + 9);
    if (length > 1024u)
    {
        return VM_FORMAT;
    }

    payload_size = wire_payload_size(command, length);
    if (size != payload_size + 13u)
    {
        return VM_FORMAT;
    }

    crc = get16(data + 11 + payload_size);
    if (crc != vm_custom_crc16(data + 2, 9u + payload_size))
    {
        return VM_FORMAT;
    }

    message->version = data[2];
    message->command = command;
    message->sequence = data[4];
    message->address = get32(data + 5);
    message->length = length;
    message->payload = payload_size > 0u ? data + 11 : NULL;

    return VM_OK;
}

vm_status_t vm_custom_parser_create(vm_custom_parser_t **out)
{
    vm_custom_parser_t *parser;

    if (!out)
    {
        return VM_INVALID;
    }

    *out = NULL;
    parser = (vm_custom_parser_t *)calloc(1, sizeof(*parser));
    if (!parser)
    {
        return VM_NOMEM;
    }

    parser->capacity = 2048u;
    parser->buffer = (uint8_t *)malloc(parser->capacity);
    if (!parser->buffer)
    {
        free(parser);
        return VM_NOMEM;
    }

    *out = parser;
    return VM_OK;
}

void vm_custom_parser_destroy(vm_custom_parser_t *parser)
{
    if (parser)
    {
        free(parser->buffer);
        free(parser);
    }
}

vm_status_t vm_custom_parser_feed(
    vm_custom_parser_t *parser,
    const uint8_t *data,
    size_t size,
    vm_status_t (*callback)(void *, const vm_custom_message_t *),
    void *context)
{
    vm_custom_message_t message;
    vm_status_t status;

    if (!parser || (!data && size) || !callback)
    {
        return VM_INVALID;
    }

    if (size > parser->capacity - parser->size)
    {
        return VM_NOMEM;
    }

    memcpy(parser->buffer + parser->size, data, size);
    parser->size += size;

    for (;;)
    {
        while (parser->size >= 2u && (parser->buffer[0] != VM_CUSTOM_SOF0 ||
                                      parser->buffer[1] != VM_CUSTOM_SOF1))
        {
            memmove(parser->buffer, parser->buffer + 1, --parser->size);
        }

        if (parser->size < 11u)
        {
            return VM_OK;
        }

        {
            uint8_t command = parser->buffer[3];
            uint16_t length = get16(parser->buffer + 9);
            size_t total;

            if (length > 1024u)
            {
                memmove(parser->buffer, parser->buffer + 2, parser->size - 2u);
                parser->size -= 2u;
                continue;
            }

            total = wire_payload_size(command, length) + 13u;
            if (total > parser->capacity)
            {
                return VM_FORMAT;
            }

            if (parser->size < total)
            {
                return VM_OK;
            }

            status = vm_custom_decode(parser->buffer, total, &message);
            if (status != VM_OK)
            {
                memmove(parser->buffer, parser->buffer + 2, parser->size - 2u);
                parser->size -= 2u;
                continue;
            }

            status = callback(context, &message);
            memmove(
                parser->buffer, parser->buffer + total, parser->size - total);
            parser->size -= total;
            if (status != VM_OK)
            {
                return status;
            }
        }
    }
}
