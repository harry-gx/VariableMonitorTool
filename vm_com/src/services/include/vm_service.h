/*
 * 文件说明：Services 总入口，负责挂接自定义协议、XCP、UDS 等上层服务。
 * 所属模块：通信模块 / Services 层。
 * 设计要点：COM 层只和本文件声明的服务管理器交互，不直接创建或调用具体协议服务。
 */

#ifndef VM_SERVICE_H
#define VM_SERVICE_H

#include "vm_custom_protocol.h"
#include "vm_pdur.h"

VM_BEGIN

/* 类型说明：Services 管理器，内部保存各协议服务实例。 */
typedef struct vm_service_manager vm_service_manager_t;

/* 类型说明：自定义协议消息上报回调。
 * 输入：context，调用方上下文；message，已解析完成的自定义协议消息。
 * 输出：调用方按消息内容更新 pending 表或上报 UI 事件。
 * 返回：VM_OK 表示消息被接收，其它状态码表示调用方拒绝该消息。
 */
typedef vm_status_t (*vm_service_custom_message_fn)(
    void *context,
    const vm_custom_message_t *message);

/**
 * 函数说明：创建 Services 管理器，并挂接第一阶段支持的自定义协议服务。
 * 输入：pdur，PduR 控制块；device_type，下层设备类型；lower_layer，下层 IF/TP 对象；
 *       lower_device，下层具体设备对象；custom_callback，自定义协议消息回调；
 *       custom_context，自定义协议消息回调上下文；out_manager，输出管理器指针。
 * 输出：成功时 *out_manager 指向已完成服务挂接的管理器。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误、内存不足或服务注册失败。
 */
vm_status_t vm_service_manager_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    void *lower_layer,
    void *lower_device,
    vm_service_custom_message_fn custom_callback,
    void *custom_context,
    vm_service_manager_t **out_manager);

/**
 * 函数说明：销毁 Services 管理器。
 * 输入：manager，待销毁的管理器。
 * 输出：已挂接的协议服务全部注销并释放。
 * 返回：无。
 */
void vm_service_manager_destroy(vm_service_manager_t *manager);

/**
 * 函数说明：通过 Services 层发起自定义协议读取请求。
 * 输入：manager，Services 管理器；sequence，请求序号；address，目标地址；length，读取长度。
 * 输出：请求被送入自定义协议服务编码，再经 PduR 路由到下层。
 * 返回：VM_OK 表示成功，其它状态码表示服务未挂接或发送失败。
 */
vm_status_t vm_service_custom_read(vm_service_manager_t *manager,
                                   uint8_t sequence,
                                   uint32_t address,
                                   uint16_t length);

/**
 * 函数说明：通过 Services 层发起自定义协议写入请求。
 * 输入：manager，Services 管理器；sequence，请求序号；address，目标地址；
 *       data，待写入的字节数据；length，写入长度。
 * 输出：请求被送入自定义协议服务编码，再经 PduR 路由到下层。
 * 返回：VM_OK 表示成功，其它状态码表示服务未挂接、参数错误或发送失败。
 */
vm_status_t vm_service_custom_write(vm_service_manager_t *manager,
                                    uint8_t sequence,
                                    uint32_t address,
                                    const uint8_t *data,
                                    uint16_t length);

VM_END

#endif
