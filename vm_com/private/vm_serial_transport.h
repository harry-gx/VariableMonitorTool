/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_SERIAL_TRANSPORT_H
#define VM_SERIAL_TRANSPORT_H
#include "vm_transport.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：device，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *device;
    /* 变量说明：baudrate，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t baudrate;
    /* 变量说明：data_bits，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t data_bits;
    /* 变量说明：stop_bits，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t stop_bits;
    /* 变量说明：parity，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t parity;
    /* 变量说明：flow_control，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t flow_control;
} vm_serial_config_t;
enum
{
    VM_SERIAL_PARITY_NONE = 0,
    VM_SERIAL_PARITY_ODD = 1,
    VM_SERIAL_PARITY_EVEN = 2
};
enum
{
    VM_SERIAL_FLOW_NONE = 0,
    VM_SERIAL_FLOW_RTS_CTS = 1,
    VM_SERIAL_FLOW_XON_XOFF = 2
};
static inline vm_status_t vm_serial_config_validate(const vm_serial_config_t *c)
{
    if (!c || !c->device || !*c->device || !c->baudrate)
    {
        return VM_INVALID;
    }
    if (c->data_bits < 5 || c->data_bits > 8 ||
        (c->stop_bits != 1 && c->stop_bits != 2))
    {
        return VM_FORMAT;
    }
    if (c->parity > VM_SERIAL_PARITY_EVEN ||
        c->flow_control > VM_SERIAL_FLOW_XON_XOFF)
    {
        return VM_FORMAT;
    }
    return VM_OK;
}
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_serial_transport vm_serial_transport_t;
/**
 * 函数说明：vm_serial_transport_create，创建并初始化对象。
 * 输入：config：设备连接参数或模块初始化参数。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_serial_transport_create(const vm_serial_config_t *config,
                                       vm_serial_transport_t **out);
/**
 * 函数说明：vm_serial_transport_destroy，销毁对象并释放相关资源。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_serial_transport_destroy(vm_serial_transport_t *transport);
/**
 * 函数说明：vm_serial_transport_base，执行本模块对应功能逻辑。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
vm_transport_t *vm_serial_transport_base(vm_serial_transport_t *transport);
/**
 * 函数说明：vm_serial_transport_receive_once，执行本模块对应功能逻辑。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。；wait_ms：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_serial_transport_receive_once(vm_serial_transport_t *transport,
                                             uint32_t wait_ms);
VM_END
#endif
