/*
 * 文件说明：自定义变量监控协议服务，封装协议编解码和 PduR 收发。
 * 所属模块：通信模块 / Services 层。
 * 设计要点：COM 层只调用服务级读写接口，协议拆包、组包和路由细节留在服务内部。
 */

#ifndef VM_CUSTOM_SERVICE_H
#define VM_CUSTOM_SERVICE_H

#include "vm_custom_protocol.h"
#include "vm_pdur.h"

VM_BEGIN

/* 类型说明：自定义协议服务对象。 */
typedef struct vm_custom_service vm_custom_service_t;

/* 类型说明：自定义协议消息回调。
 * 输入：context，调用方上下文；message，解析完成的协议消息。
 * 输出：调用方按消息更新 pending 表或 UI 事件。
 * 返回：VM_OK 表示处理成功，其它状态码表示调用方拒绝该消息。
 */
typedef vm_status_t (*vm_custom_service_message_fn)(
    void *context,
    const vm_custom_message_t *message);

/**
 * 函数说明：创建并注册自定义协议服务。
 * 输入：pdur，PduR 控制块；device_type，服务绑定的设备类型；lower_layer，下层 IF/TP 对象；
 *       lower_device，下层具体设备对象；callback，协议消息回调；callback_context，回调上下文；
 *       out_service，输出服务对象。
 * 输出：成功时服务注册到 PduR，后续 PduR 输入会进入该服务解析。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误、内存不足或注册失败。
 */
vm_status_t vm_custom_service_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    void *lower_layer,
    void *lower_device,
    vm_custom_service_message_fn callback,
    void *callback_context,
    vm_custom_service_t **out_service);

/**
 * 函数说明：销毁自定义协议服务。
 * 输入：service，待销毁服务对象。
 * 输出：服务从 PduR 注销，协议解析器释放。
 * 返回：无。
 */
void vm_custom_service_destroy(vm_custom_service_t *service);

/**
 * 函数说明：发送自定义协议读请求。
 * 输入：service，自定义协议服务；sequence，协议序号；address，目标地址；length，读取长度。
 * 输出：读请求被编码并通过 PduR 发送到下层。
 * 返回：VM_OK 表示成功，其它状态码表示编码或发送失败。
 */
vm_status_t vm_custom_service_send_read(vm_custom_service_t *service,
                                        uint8_t sequence,
                                        uint32_t address,
                                        uint16_t length);

/**
 * 函数说明：发送自定义协议写请求。
 * 输入：service，自定义协议服务；sequence，协议序号；address，目标地址；
 *       data，写入数据；length，写入长度。
 * 输出：写请求被编码并通过 PduR 发送到下层。
 * 返回：VM_OK 表示成功，其它状态码表示编码或发送失败。
 */
vm_status_t vm_custom_service_send_write(vm_custom_service_t *service,
                                         uint8_t sequence,
                                         uint32_t address,
                                         const uint8_t *data,
                                         uint16_t length);

VM_END

#endif
