/*
 * 文件说明：PduR 路由层接口，根据设备类型把下层数据分发到对应服务，并把服务输出路由回下层。
 * 所属模块：通信模块 / PduR 层。
 * 设计要点：串口第一阶段由 IF 直接路由到服务；CAN/CANFD 后续接入 TP 后再路由到服务。
 */

#ifndef VM_PDUR_H
#define VM_PDUR_H

#include "vm_list.h"
#include "vm_status.h"

VM_BEGIN

/* 常量说明：自定义协议服务 ID，用于在 PduR 服务表中标识变量监控协议。 */
#define VM_PDUR_SERVICE_CUSTOM (1u)

/* 类型说明：PduR 支持的下层设备类型。 */
typedef enum
{
    VM_PDUR_DEVICE_SERIAL = 0,
    VM_PDUR_DEVICE_CAN,
    VM_PDUR_DEVICE_CANFD,
    VM_PDUR_DEVICE_ETHERNET
} vm_pdur_device_type_t;

/* 类型说明：PduR 控制块。 */
typedef struct vm_pdur vm_pdur_t;

/* 类型说明：PduR 路由上下文，一次输入或输出都通过该对象描述。 */
typedef struct
{
    /* 变量说明：下层设备类型，决定 PduR 走串口 IF、CAN TP 或网口 IF。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：服务 ID，第一阶段串口自定义协议使用 VM_PDUR_SERVICE_CUSTOM。 */
    uint16_t service_id;
    /* 变量说明：下层抽象层对象，例如 vm_serial_if_t。 */
    void *lower_layer;
    /* 变量说明：下层具体设备对象，例如 vm_mcal_serial_device_t。 */
    void *lower_device;
    /* 变量说明：接收方向原始数据。 */
    const uint8_t *rx_data;
    /* 变量说明：接收方向原始数据长度。 */
    size_t rx_size;
    /* 变量说明：发送方向原始数据。 */
    const uint8_t *tx_data;
    /* 变量说明：发送方向原始数据长度。 */
    size_t tx_size;
    /* 变量说明：通道号，串口当前固定为 0，CAN 后续用于通道/帧路由。 */
    uint32_t channel;
} vm_pdur_context_t;

/* 类型说明：服务匹配函数。
 * 输入：context，服务私有上下文；pdur_context，本次路由上下文。
 * 输出：无。
 * 返回：1 表示该服务接收本次数据，0 表示不匹配。
 */
typedef uint8_t (*vm_pdur_service_match_fn)(
    void *context,
    const vm_pdur_context_t *pdur_context);

/* 类型说明：服务处理函数。
 * 输入：context，服务私有上下文；pdur_context，本次路由上下文。
 * 输出：服务自行处理接收数据，必要时可通过 PduR 输出响应。
 * 返回：VM_OK 表示处理成功，其它状态码表示服务拒绝或处理失败。
 */
typedef vm_status_t (*vm_pdur_service_process_fn)(
    void *context,
    vm_pdur_context_t *pdur_context);

/* 类型说明：PduR 服务节点，注册后由 PduR 根据设备类型和 match 函数分发。 */
typedef struct vm_pdur_service
{
    /* 变量说明：侵入式链表节点，用于挂接到 PduR 服务表。 */
    list_head_t node;
    /* 变量说明：服务 ID。 */
    uint16_t service_id;
    /* 变量说明：服务接收的设备类型。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：服务匹配函数，NULL 表示同设备类型全部接收。 */
    vm_pdur_service_match_fn match;
    /* 变量说明：服务处理函数。 */
    vm_pdur_service_process_fn process;
    /* 变量说明：服务私有上下文。 */
    void *context;
} vm_pdur_service_t;

/**
 * 函数说明：创建 PduR 控制块。
 * 输入：out_pdur，输出控制块指针。
 * 输出：成功时 *out_pdur 指向空服务表的 PduR 对象。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误或内存不足。
 */
vm_status_t vm_pdur_create(vm_pdur_t **out_pdur);

/**
 * 函数说明：销毁 PduR 控制块。
 * 输入：pdur，待销毁控制块。
 * 输出：释放 PduR 控制块；服务节点由服务模块自行管理。
 * 返回：无。
 */
void vm_pdur_destroy(vm_pdur_t *pdur);

/**
 * 函数说明：注册服务到 PduR。
 * 输入：pdur，PduR 控制块；service，待注册服务节点。
 * 输出：service 加入 PduR 服务链表。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误或重复注册。
 */
vm_status_t vm_pdur_register_service(vm_pdur_t *pdur,
                                     vm_pdur_service_t *service);

/**
 * 函数说明：从 PduR 注销服务。
 * 输入：pdur，PduR 控制块；service，待注销服务节点。
 * 输出：service 从 PduR 服务链表移除。
 * 返回：VM_OK 表示成功，其它状态码表示未找到或参数错误。
 */
vm_status_t vm_pdur_unregister_service(vm_pdur_t *pdur,
                                       vm_pdur_service_t *service);

/**
 * 函数说明：PduR 输入入口，把下层收到的数据路由到服务。
 * 输入：pdur，PduR 控制块；context，本次接收上下文。
 * 输出：匹配服务被调用。
 * 返回：VM_OK 表示至少一个服务处理成功，VM_NOT_FOUND 表示没有匹配服务。
 */
vm_status_t vm_pdur_input(vm_pdur_t *pdur, vm_pdur_context_t *context);

/**
 * 函数说明：PduR 输出入口，把服务生成的数据路由到对应下层。
 * 输入：pdur，PduR 控制块；context，本次发送上下文。
 * 输出：数据通过串口 IF、CAN TP 或网口 IF 发送。
 * 返回：VM_OK 表示发送成功，其它状态码表示下层未支持或发送失败。
 */
vm_status_t vm_pdur_output(vm_pdur_t *pdur,
                           const vm_pdur_context_t *context);

VM_END

#endif
