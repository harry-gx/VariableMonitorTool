/*
 * 文件说明：自定义监控标定协议实现，负责帧编码、CRC 校验、流式拆包和消息解析。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_custom_protocol.h"

#include <string.h>

/* 变量说明：兼容旧 create/destroy 接口的静态解析器实例，测试代码可继续使用指针形式。 */
static vm_custom_parser_t g_vm_custom_parser_instance;
/* 变量说明：静态解析器占用标志，防止旧接口重复获取同一个解析器。 */
static uint8_t g_vm_custom_parser_allocated;

/**
 * 函数说明：put16，执行本模块对应功能逻辑。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。；value：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void put16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

/**
 * 函数说明：put32，执行本模块对应功能逻辑。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。；value：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void put32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

/**
 * 函数说明：get16，执行本模块对应功能逻辑。
 * 输入：data：输入或输出的原始字节缓冲区。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static uint16_t get16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

/**
 * 函数说明：get32，执行本模块对应功能逻辑。
 * 输入：data：输入或输出的原始字节缓冲区。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static uint32_t get32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

/**
 * 函数说明：wire_payload_size，加载外部文件或配置。
 * 输入：command：函数输入参数，参与本函数的计算、查找或状态更新。；length：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
static size_t wire_payload_size(uint8_t command, uint16_t length)
{
    return command == VM_CUSTOM_READ ? 0u : (size_t)length;
}

/**
 * 函数说明：vm_custom_crc16，计算或校验 CRC。
 * 输入：data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static uint16_t vm_custom_crc16(const uint8_t *data, size_t size)
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

/**
 * 函数说明：vm_custom_make_read，读取数据或发起读取请求。
 * 输入：sequence：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；length：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。；capacity：函数输入参数，参与本函数的计算、查找或状态更新。；written：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_custom_make_write，写入数据或发起标定请求。
 * 输入：sequence：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；data：输入或输出的原始字节缓冲区。；length：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。；capacity：函数输入参数，参与本函数的计算、查找或状态更新。；written：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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


/**
 * 函数说明：vm_custom_encode，编码数据到传输格式。
 * 输入：message：协议解析后的消息对象。；out：输出对象或结果指针，函数成功时写入有效值。；capacity：函数输入参数，参与本函数的计算、查找或状态更新。；written：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_custom_decode，解码传输数据。
 * 输入：data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。；message：协议解析后的消息对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

vm_status_t vm_custom_parser_init(vm_custom_parser_t *parser)
{
    vm_status_t status;

    if (parser == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        (void)memset(parser, 0, sizeof(*parser));
        status = VM_OK;
    }

    return status;
}

void vm_custom_parser_reset(vm_custom_parser_t *parser)
{
    if (parser != NULL)
    {
        (void)vm_custom_parser_init(parser);
    }
}

/**
 * 函数说明：vm_custom_parser_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_custom_parser_create(vm_custom_parser_t **out)
{
    vm_status_t status;

    if (out == NULL)
    {
        status = VM_INVALID;
    }
    else if (g_vm_custom_parser_allocated != 0u)
    {
        *out = NULL;
        status = VM_BUSY;
    }
    else
    {
        status = vm_custom_parser_init(&g_vm_custom_parser_instance);
        if (status == VM_OK)
        {
            g_vm_custom_parser_allocated = 1u;
            *out = &g_vm_custom_parser_instance;
        }
        else
        {
            *out = NULL;
        }
    }

    return status;
}

/**
 * 函数说明：vm_custom_parser_destroy，销毁对象并释放相关资源。
 * 输入：parser：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_custom_parser_destroy(vm_custom_parser_t *parser)
{
    if (parser == &g_vm_custom_parser_instance)
    {
        vm_custom_parser_reset(parser);
        g_vm_custom_parser_allocated = 0u;
    }
}

/**
 * 函数说明：vm_custom_parser_feed，解析输入数据并生成内部结果。
 * 输入：parser：函数输入参数，参与本函数的计算、查找或状态更新。；data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。；callback：事件回调函数指针。；context：回调上下文指针，由调用方传入并在回调中原样返回。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_custom_parser_feed(
    vm_custom_parser_t *parser,
    const uint8_t *data,
    size_t size,
    vm_status_t (*callback)(void *, const vm_custom_message_t *),
    void *context)
{
    vm_custom_message_t message;
    vm_status_t status;

    if ((parser == NULL) || ((data == NULL) && (size > 0u)) ||
        (callback == NULL))
    {
        return VM_INVALID;
    }

    if (size > ((size_t)VM_CUSTOM_PARSER_BUFFER_SIZE - parser->size))
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
            if (total > (size_t)VM_CUSTOM_PARSER_BUFFER_SIZE)
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
