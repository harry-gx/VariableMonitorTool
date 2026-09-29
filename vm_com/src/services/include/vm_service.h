/*
 * 文件说明：Services 层总入口，只负责把各协议服务节点挂接到 PduR。
 * 所属模块：通信模块 / Services 层。
 * 设计要点：本层不参与 COM 与协议之间的数据交换，COM 和协议服务只通过 PduR 通信。
 */

#ifndef VM_SERVICE_H
#define VM_SERVICE_H

#include "vm_pdur.h"

VM_BEGIN

/* 类型说明：Services 管理器，内部保存已经注册到 PduR 的协议服务实例。 */
typedef struct vm_service_manager vm_service_manager_t;

/**
 * 函数说明：创建 Services 管理器，并把当前支持的协议服务节点注册到 PduR。
 * 输入：pdur，PduR 控制块；device_type，下层设备类型；channel，通信通道号；out_manager，输出管理器指针。
 * 输出：成功时 custom 服务节点已注册到 PduR，后续 COM 请求和下层数据均由 PduR 路由到协议服务。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误、内存不足或服务注册失败。
 */
vm_status_t vm_service_manager_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    uint32_t channel,
    vm_service_manager_t **out_manager);

/**
 * 函数说明：销毁 Services 管理器。
 * 输入：manager，待销毁的管理器。
 * 输出：已挂接的协议服务从 PduR 注销并释放。
 * 返回：无。
 */
void vm_service_manager_destroy(vm_service_manager_t *manager);

VM_END

#endif
