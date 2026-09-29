/*
 * 文件说明：自定义变量监控协议服务节点接口，供 Services 层注册到 PduR。
 * 所属模块：通信模块 / Services 层 / Custom 协议。
 * 设计要点：本文件只暴露协议节点创建和销毁接口，协议收发均由 PduR 调度。
 */

#ifndef VM_CUSTOM_SERVICE_H
#define VM_CUSTOM_SERVICE_H

#include "vm_pdur.h"

VM_BEGIN

/* 类型说明：自定义协议服务对象。 */
typedef struct vm_custom_service vm_custom_service_t;

/**
 * 函数说明：创建并注册自定义协议服务节点。
 * 输入：pdur，PduR 控制块；device_type，服务绑定的设备类型；channel，服务绑定的通道号；out_service，输出服务对象。
 * 输出：成功时服务节点注册到 PduR，后续 COM 请求和下层输入由 PduR 分发到该服务。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误、内存不足或注册失败。
 */
vm_status_t vm_custom_service_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    uint32_t channel,
    vm_custom_service_t **out_service);

/**
 * 函数说明：销毁自定义协议服务。
 * 输入：service，待销毁服务对象。
 * 输出：服务从 PduR 注销，协议解析器释放。
 * 返回：无。
 */
void vm_custom_service_destroy(vm_custom_service_t *service);

VM_END

#endif
