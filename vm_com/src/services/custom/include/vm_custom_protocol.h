/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_CUSTOM_PROTOCOL_H
#define VM_CUSTOM_PROTOCOL_H
#include "vm_status.h"
VM_BEGIN
/* 常量说明：VM_CUSTOM_SOF0 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_CUSTOM_SOF0 0xAAu
/* 常量说明：VM_CUSTOM_SOF1 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_CUSTOM_SOF1 0x55u
enum
{
    VM_CUSTOM_READ = 1,
    VM_CUSTOM_WRITE = 2,
    VM_CUSTOM_READ_RESPONSE = 0x81,
    VM_CUSTOM_WRITE_RESPONSE = 0x82,
    VM_CUSTOM_ERROR = 0xE0
};
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：version，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t version;
    /* 变量说明：command，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t command;
    /* 变量说明：sequence，协议序号，用于匹配请求和响应。 */
    uint8_t sequence;
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint32_t address;
    /* 变量说明：payload，保存当前对象运行所需的状态、参数或缓存数据。 */
    const uint8_t *payload;
    /* 变量说明：length，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint16_t length;
} vm_custom_message_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_custom_parser vm_custom_parser_t;
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
                                size_t *written);
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
                                 size_t *written);
/**
 * 函数说明：vm_custom_crc16，计算或校验 CRC。
 * 输入：data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
uint16_t vm_custom_crc16(const uint8_t *data, size_t size);
/**
 * 函数说明：vm_custom_encoded_size，编码数据到传输格式。
 * 输入：message：协议解析后的消息对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_custom_encoded_size(const vm_custom_message_t *message);
/**
 * 函数说明：vm_custom_encode，编码数据到传输格式。
 * 输入：message：协议解析后的消息对象。；out：输出对象或结果指针，函数成功时写入有效值。；capacity：函数输入参数，参与本函数的计算、查找或状态更新。；written：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_custom_encode(const vm_custom_message_t *message,
                             uint8_t *out,
                             size_t capacity,
                             size_t *written);
/**
 * 函数说明：vm_custom_decode，解码传输数据。
 * 输入：data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。；message：协议解析后的消息对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_custom_decode(const uint8_t *data,
                             size_t size,
                             vm_custom_message_t *message);
/**
 * 函数说明：vm_custom_parser_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_custom_parser_create(vm_custom_parser_t **out);
/**
 * 函数说明：vm_custom_parser_destroy，销毁对象并释放相关资源。
 * 输入：parser：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_custom_parser_destroy(vm_custom_parser_t *parser);
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
    void *context);
VM_END
#endif
