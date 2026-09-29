/*
 * 文件说明：PduR 路由层接口，统一管理下层通信通道、上层服务协议和 COM 之间的数据路由。
 * 所属模块：通信模块 / PduR 层。
 * 设计要点：下层 IF/TP、上层 CUSTOM/XCP/UDS、COM 均只通过 PduR 交换数据。
 */

#ifndef VM_PDUR_H
#define VM_PDUR_H

#include "vm_list.h"
#include "vm_status.h"

VM_BEGIN

/* 常量说明：服务 ID 为 0 时表示尚未识别具体协议，PduR 交给服务 match 函数进一步判断。 */
#define VM_PDUR_SERVICE_ANY (0u)

/* 常量说明：自定义协议服务 ID，用于在 PduR 服务表中标识变量监控协议。 */
#define VM_PDUR_SERVICE_CUSTOM (1u)

/* 常量说明：默认通信通道号，第一阶段串口链路固定使用该通道。 */
#define VM_PDUR_CHANNEL_DEFAULT (0u)

/* 常量说明：COM 与协议服务之间的通用变量读写命令。 */
#define VM_PDUR_COMMAND_READ (1u)
#define VM_PDUR_COMMAND_WRITE (2u)
#define VM_PDUR_COMMAND_READ_RESPONSE (0x81u)
#define VM_PDUR_COMMAND_WRITE_RESPONSE (0x82u)
#define VM_PDUR_COMMAND_ERROR (0xE0u)

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

/* 类型说明：PduR 下层路由节点，内部保存 IF/TP 通道和读写回调。 */
typedef struct vm_pdur_lower_route vm_pdur_lower_route_t;

/* 类型说明：COM 与协议服务之间的逻辑消息。
 * 说明：该结构只表达变量监控工具通用读写语义，不携带具体协议帧格式。
 */
typedef struct
{
    /* 变量说明：目标下层设备类型，用于选择协议服务和下层链路。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：目标服务 ID，例如 VM_PDUR_SERVICE_CUSTOM。 */
    uint16_t service_id;
    /* 变量说明：通道号，用于区分同类型多通道链路。 */
    uint32_t channel;
    /* 变量说明：通用命令，取 VM_PDUR_COMMAND_xxx。 */
    uint8_t command;
    /* 变量说明：请求或响应序号。 */
    uint8_t sequence;
    /* 变量说明：目标 MCU 地址。 */
    uint32_t address;
    /* 变量说明：载荷指针，读请求可为空，写请求和读响应指向数据。 */
    const uint8_t *payload;
    /* 变量说明：载荷长度或读请求长度。 */
    uint16_t length;
} vm_pdur_message_t;

/* 类型说明：PduR 下层读取函数。
 * 输入：lower_layer，下层 IF/TP 对象；lower_device，下层具体设备对象；buffer，接收缓冲区；capacity，缓冲区容量。
 * 输出：read_size 返回本次读取到的字节数。
 * 返回：VM_OK 表示读到或完成读取，VM_NOT_FOUND 表示暂无数据，其它状态码表示读取失败。
 */
typedef vm_status_t (*vm_pdur_lower_read_fn)(
    void *lower_layer,
    void *lower_device,
    uint8_t *buffer,
    size_t capacity,
    size_t *read_size);

/* 类型说明：PduR 下层发送函数。
 * 输入：lower_layer，下层 IF/TP 对象；lower_device，下层具体设备对象；data，待发送数据；size，待发送长度。
 * 输出：数据由下层通道发出。
 * 返回：VM_OK 表示发送成功，其它状态码表示发送失败。
 */
typedef vm_status_t (*vm_pdur_lower_write_fn)(
    void *lower_layer,
    void *lower_device,
    const uint8_t *data,
    size_t size);

/* 类型说明：下层路由注册配置。 */
typedef struct
{
    /* 变量说明：下层设备类型，决定服务匹配和输出路由的类型。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：通道号，用于区分同类型下层设备，例如多路串口或多路 CAN。 */
    uint32_t channel;
    /* 变量说明：下层抽象层对象，例如 vm_serial_if_t。 */
    void *lower_layer;
    /* 变量说明：下层具体设备对象，例如运行时串口设备。 */
    void *lower_device;
    /* 变量说明：从下层读取原始字节流的回调。 */
    vm_pdur_lower_read_fn read;
    /* 变量说明：向下层发送原始字节流的回调。 */
    vm_pdur_lower_write_fn write;
} vm_pdur_lower_config_t;

/* 类型说明：PduR 下层路由静态节点。
 * 说明：节点由调用方全局静态定义，PduR 注册时只挂接链表，不申请内存。
 */
struct vm_pdur_lower_route
{
    /* 变量说明：侵入式链表节点，用于挂接到 PduR 下层路由表。 */
    list_head_t node;
    /* 变量说明：下层通道配置，包含设备类型、通道号和读写回调。 */
    vm_pdur_lower_config_t config;
    /* 变量说明：注册状态，1 表示节点已经挂接到 PduR。 */
    uint8_t registered;
};

/* 类型说明：PduR 路由上下文，一次下层输入或下层输出都通过该对象描述。 */
typedef struct
{
    /* 变量说明：下层设备类型，决定输入匹配和输出路由。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：服务 ID，接收方向可为 VM_PDUR_SERVICE_ANY，由服务自行匹配。 */
    uint16_t service_id;
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
 * 输出：服务自行处理下层收到的数据，必要时可通过 PduR 输出响应。
 * 返回：VM_OK 表示处理成功，其它状态码表示服务拒绝或处理失败。
 */
typedef vm_status_t (*vm_pdur_service_process_fn)(
    void *context,
    vm_pdur_context_t *pdur_context);

/* 类型说明：COM 请求服务处理函数。
 * 输入：context，服务私有上下文；message，COM 通过 PduR 下发的逻辑消息。
 * 输出：服务把逻辑消息组包后经 PduR 下发到底层。
 * 返回：VM_OK 表示处理成功，其它状态码表示服务拒绝或处理失败。
 */
typedef vm_status_t (*vm_pdur_service_request_fn)(
    void *context,
    const vm_pdur_message_t *message);

/* 类型说明：协议服务发给 COM 的消息回调。
 * 输入：context，COM 私有上下文；message，服务通过 PduR 上报的逻辑消息。
 * 输出：COM 根据消息更新 pending 表并上报 UI。
 * 返回：VM_OK 表示 COM 已接收消息，其它状态码表示拒绝或处理失败。
 */
typedef vm_status_t (*vm_pdur_com_indication_fn)(
    void *context,
    const vm_pdur_message_t *message);

/* 类型说明：COM 节点注册配置。 */
typedef struct
{
    /* 变量说明：协议服务上报消息时调用的 COM 回调。 */
    vm_pdur_com_indication_fn indication;
    /* 变量说明：COM 回调上下文。 */
    void *context;
} vm_pdur_com_config_t;

/* 类型说明：PduR 服务节点，注册后由 PduR 根据设备类型、通道和服务 ID 分发。 */
typedef struct vm_pdur_service
{
    /* 变量说明：侵入式链表节点，用于挂接到 PduR 服务表。 */
    list_head_t node;
    /* 变量说明：服务 ID。 */
    uint16_t service_id;
    /* 变量说明：服务接收的设备类型。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：服务绑定的通道号。 */
    uint32_t channel;
    /* 变量说明：下层数据匹配函数，NULL 表示同设备类型和通道全部接收。 */
    vm_pdur_service_match_fn match;
    /* 变量说明：下层数据处理函数。 */
    vm_pdur_service_process_fn process;
    /* 变量说明：COM 逻辑请求处理函数。 */
    vm_pdur_service_request_fn request;
    /* 变量说明：服务私有上下文。 */
    void *context;
    /* 变量说明：注册状态，1 表示服务节点已经挂接到 PduR。 */
    uint8_t registered;
} vm_pdur_service_t;

vm_status_t vm_pdur_create(vm_pdur_t **out_pdur);
void vm_pdur_destroy(vm_pdur_t *pdur);

vm_status_t vm_pdur_register_lower_route(vm_pdur_t *pdur,
                                         vm_pdur_lower_route_t *route);
vm_status_t vm_pdur_unregister_lower_route(vm_pdur_t *pdur,
                                           vm_pdur_lower_route_t *route);

vm_status_t vm_pdur_register_com_route(vm_pdur_t *pdur,
                                       const vm_pdur_com_config_t *config);
void vm_pdur_unregister_com_route(vm_pdur_t *pdur);

vm_status_t vm_pdur_register_service(vm_pdur_t *pdur,
                                     vm_pdur_service_t *service);
vm_status_t vm_pdur_unregister_service(vm_pdur_t *pdur,
                                       vm_pdur_service_t *service);

vm_status_t vm_pdur_poll(vm_pdur_t *pdur, uint8_t budget);
vm_status_t vm_pdur_input(vm_pdur_t *pdur, vm_pdur_context_t *context);
vm_status_t vm_pdur_output(vm_pdur_t *pdur,
                           const vm_pdur_context_t *context);

vm_status_t vm_pdur_service_request(vm_pdur_t *pdur,
                                    const vm_pdur_message_t *message);
vm_status_t vm_pdur_com_indicate(vm_pdur_t *pdur,
                                 const vm_pdur_message_t *message);

VM_END

#endif
