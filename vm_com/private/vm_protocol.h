/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_PROTOCOL_H
#define VM_PROTOCOL_H
#include "vm_transport.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_protocol vm_protocol_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：data，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t *data;
    /* 变量说明：size，数据长度，单位为字节。 */
    size_t size;
    /* 变量说明：capacity，动态数组容量。 */
    size_t capacity;
    /* 变量说明：channel，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t channel;
} vm_protocol_packet_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：protocol：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*reset)(vm_protocol_t *protocol);
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：packet：底层传输收到的数据包。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*on_transport_packet)(vm_protocol_t *protocol,
                                       const vm_transport_packet_t *packet);
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：size：数据长度或缓冲区容量。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*read_memory)(vm_protocol_t *protocol,
                               uint64_t address,
                               uint8_t *data,
                               size_t size);
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：size：数据长度或缓冲区容量。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*write_memory)(vm_protocol_t *protocol,
                                uint64_t address,
                                const uint8_t *data,
                                size_t size);
} vm_protocol_ops_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_protocol
{
    /* 变量说明：ops，操作函数表指针。 */
    const vm_protocol_ops_t *ops;
    /* 变量说明：transport，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_t *transport;
    /* 变量说明：context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *context;
};
static inline vm_status_t vm_protocol_send(vm_protocol_t *p,
                                           const uint8_t *data,
                                           size_t size,
                                           uint32_t channel)
{
    if (!p || (!data && size) || !p->transport || !p->transport->ops ||
        !p->transport->ops->send)
    {
        return VM_INVALID;
    }
    return p->transport->ops->send(p->transport, data, size, channel);
}
VM_END
#endif
