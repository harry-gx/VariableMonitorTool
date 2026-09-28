/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_TRANSPORT_H
#define VM_TRANSPORT_H
#include "vm_status.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_transport vm_transport_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：data，保存当前对象运行所需的状态、参数或缓存数据。 */
    const uint8_t *data;
    /* 变量说明：size，数据长度，单位为字节。 */
    size_t size;
    /* 变量说明：channel，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t channel;
    /* 变量说明：timestamp，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t timestamp;
} vm_transport_packet_t;
typedef vm_status_t (*vm_transport_rx_fn)(void *context,
                                          const vm_transport_packet_t *packet);
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*open)(vm_transport_t *transport);
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*close)(vm_transport_t *transport);
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：channel：函数输入参数，参与本函数的计算、查找或状态更新。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*send)(vm_transport_t *transport,
                        const uint8_t *data,
                        size_t size,
                        uint32_t channel);
    /**
     * 函数说明：vm_status_t，执行本模块对应功能逻辑。
     * 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。
     * 输出：通过返回值、对象成员或输出参数反馈处理结果。
     * 返回：返回执行结果，具体含义由调用方按接口约定解释。
     */
    vm_status_t (*set_rx)(vm_transport_t *transport,
                          vm_transport_rx_fn callback,
                          void *context);
} vm_transport_ops_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_transport
{
    /* 变量说明：ops，操作函数表指针。 */
    const vm_transport_ops_t *ops;
    /* 变量说明：context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *context;
    /* 变量说明：opened，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t opened;
};
static inline vm_status_t vm_transport_open(vm_transport_t *t)
{
    return t && t->ops && t->ops->open ? t->ops->open(t) : VM_INVALID;
}
static inline vm_status_t vm_transport_close(vm_transport_t *t)
{
    return t && t->ops && t->ops->close ? t->ops->close(t) : VM_INVALID;
}
static inline vm_status_t
vm_transport_send(vm_transport_t *t, const uint8_t *d, size_t n, uint32_t c)
{
    return t && t->ops && t->ops->send ? t->ops->send(t, d, n, c) : VM_INVALID;
}
VM_END
#endif
