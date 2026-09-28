/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_ROUTER_H
#define VM_ROUTER_H
#include "vm_status.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_router vm_router_t;
/* Transport identity is a session/connection token, not a driver type.
   Frame and payload borrowed only until callback returns. */
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：connection，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t connection;
    /* 变量说明：channel，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t channel;
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint32_t address;
    /* 变量说明：data，保存当前对象运行所需的状态、参数或缓存数据。 */
    const uint8_t *data;
    /* 变量说明：size，数据长度，单位为字节。 */
    size_t size;
} vm_frame_t;
typedef vm_status_t (*vm_frame_fn)(void *context, const vm_frame_t *frame);
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：route_id，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t route_id;
    /* 变量说明：connection，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t connection;
    /* 变量说明：channel，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t channel;
    /* 变量说明：rx_address，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t rx_address;
    /* 变量说明：tx_address，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t tx_address;
    /* 变量说明：protocol_receive，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_frame_fn protocol_receive;
    /* 变量说明：transport_send，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_frame_fn transport_send;
    /* 变量说明：protocol_context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *protocol_context;
    /* 变量说明：transport_context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *transport_context;
} vm_route_t;
/* All calls on one service thread. Callbacks may send, but not mutate routes. */
/**
 * 函数说明：vm_router_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_create(vm_router_t **out);
/**
 * 函数说明：vm_router_destroy，销毁对象并释放相关资源。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_destroy(vm_router_t *r);
/**
 * 函数说明：vm_router_bind，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；route：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_bind(vm_router_t *r, const vm_route_t *route);
/**
 * 函数说明：vm_router_unbind，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；route_id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_unbind(vm_router_t *r, uint64_t route_id);
/**
 * 函数说明：vm_router_receive，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；frame：协议帧或传输帧数据。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_receive(vm_router_t *r, const vm_frame_t *frame);
/**
 * 函数说明：vm_router_send，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；route_id：函数输入参数，参与本函数的计算、查找或状态更新。；data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_send(vm_router_t *r,
                           uint64_t route_id,
                           const uint8_t *data,
                           size_t size);
/**
 * 函数说明：vm_router_route_count，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_router_route_count(const vm_router_t *r);
/**
 * 函数说明：vm_router_route_at，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_router_route_at(const vm_router_t *r, size_t index, vm_route_t *out);
VM_END
#endif
