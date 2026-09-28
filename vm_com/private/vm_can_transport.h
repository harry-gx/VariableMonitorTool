/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_CAN_TRANSPORT_H
#define VM_CAN_TRANSPORT_H
#include "vm_transport.h"
VM_BEGIN
/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
typedef enum
{
    VM_CAN_VENDOR_ECANVCI = 1,
    VM_CAN_VENDOR_ZLG = 2
} vm_can_vendor_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：vendor，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_can_vendor_t vendor;
    /* 变量说明：device_type，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t device_type;
    /* 变量说明：device_index，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t device_index;
    /* 变量说明：channel_index，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t channel_index;
    /* 变量说明：nominal_bitrate，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t nominal_bitrate;
    /* 变量说明：data_bitrate，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t data_bitrate;
    /* 变量说明：fd，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t fd;
    /* 变量说明：brs，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t brs;
    /* 变量说明：listen_only，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t listen_only;
} vm_can_config_t;
static inline vm_status_t vm_can_config_validate(const vm_can_config_t *c)
{
    if (!c ||
        (c->vendor != VM_CAN_VENDOR_ECANVCI &&
         c->vendor != VM_CAN_VENDOR_ZLG) ||
        !c->nominal_bitrate)
    {
        return VM_INVALID;
    }
    if (c->fd && !c->data_bitrate)
    {
        return VM_INVALID;
    }
    if (!c->fd && (c->brs || c->data_bitrate))
    {
        return VM_FORMAT;
    }
    if (c->fd > 1 || c->brs > 1 || c->listen_only > 1)
    {
        return VM_FORMAT;
    }
    return VM_OK;
}
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：id，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t id;
    /* 变量说明：extended，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t extended;
    /* 变量说明：remote，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t remote;
    /* 变量说明：fd，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t fd;
    /* 变量说明：brs，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t brs;
    /* 变量说明：data_length，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t data_length;
    /* 变量说明：data，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t data[64];
    /* 变量说明：timestamp，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t timestamp;
} vm_can_frame_t;
static inline vm_status_t vm_can_frame_validate(const vm_can_frame_t *f)
{
    if (!f)
    {
        return VM_INVALID;
    }
    if (f->extended ? f->id > 0x1fffffffu : f->id > 0x7ffu)
    {
        return VM_FORMAT;
    }
    if (f->remote && f->data_length)
    {
        return VM_FORMAT;
    }
    if (f->data_length > (f->fd ? 64u : 8u))
    {
        return VM_FORMAT;
    }
    if (!f->fd && f->brs)
    {
        return VM_FORMAT;
    }
    return VM_OK;
}
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_can_transport vm_can_transport_t;
/**
 * 函数说明：vm_can_transport_create，创建并初始化对象。
 * 输入：config：设备连接参数或模块初始化参数。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_can_transport_create(const vm_can_config_t *config,
                                    vm_can_transport_t **out);
/**
 * 函数说明：vm_can_transport_destroy，销毁对象并释放相关资源。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_can_transport_destroy(vm_can_transport_t *transport);
/**
 * 函数说明：vm_can_transport_base，执行本模块对应功能逻辑。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
vm_transport_t *vm_can_transport_base(vm_can_transport_t *transport);
/**
 * 函数说明：vm_can_transport_receive_once，执行本模块对应功能逻辑。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。；wait_ms：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_can_transport_receive_once(vm_can_transport_t *transport,
                                          uint32_t wait_ms);
VM_END
#endif
