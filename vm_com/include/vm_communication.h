/*
 * 文件说明：通信模块对 UI 暴露的统一 COM 层接口，封装设备连接、变量读取、变量标定和事件回调。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_COMMUNICATION_H
#define VM_COMMUNICATION_H

#include "vm_status.h"

VM_BEGIN

/* 常量说明：VM_COMM_NAME_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_NAME_MAX (128u)
/* 常量说明：VM_COMM_TEXT_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_TEXT_MAX (128u)

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_comm vm_comm_t;

/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
typedef enum
{
    VM_COMM_DEVICE_SERIAL = 0,
    VM_COMM_DEVICE_CAN,
    VM_COMM_DEVICE_CANFD,
    VM_COMM_DEVICE_ETHERNET
} vm_comm_device_type_t;

/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
typedef enum
{
    VM_COMM_EVENT_VALUE = 1,
    VM_COMM_EVENT_WRITE_DONE,
    VM_COMM_EVENT_ERROR
} vm_comm_event_type_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：type，对象类型或事件类型。 */
    vm_comm_device_type_t type;
    /* 变量说明：serial_device，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *serial_device;
    /* 变量说明：serial_baudrate，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t serial_baudrate;
    /* 变量说明：serial_data_bits，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t serial_data_bits;
    /* 变量说明：serial_stop_bits，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t serial_stop_bits;
    /* 变量说明：serial_parity，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t serial_parity;
    /* 变量说明：serial_flow_control，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t serial_flow_control;
    /* 变量说明：can_adapter，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *can_adapter;
    /* 变量说明：can_channel，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t can_channel;
    /* 变量说明：can_baudrate，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t can_baudrate;
    /* 变量说明：canfd_data_baudrate，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t canfd_data_baudrate;
    /* 变量说明：network_host，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *network_host;
    /* 变量说明：network_port，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint16_t network_port;
} vm_comm_device_config_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：name，变量名、页面名或节点名。 */
    const char *name;
    /* 变量说明：type_name，变量类型名称。 */
    const char *type_name;
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint32_t address;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint16_t size;
    /* 变量说明：writable，变量是否允许写入。 */
    uint8_t writable;
    /* 变量说明：monitorable，变量是否允许加入监控表。 */
    uint8_t monitorable;
    /* 变量说明：calibratable，变量是否允许加入标定表。 */
    uint8_t calibratable;
    /* 变量说明：bit_field，是否为位字段变量。 */
    uint8_t bit_field;
    /* 变量说明：bit_offset，位字段起始 bit 偏移。 */
    uint8_t bit_offset;
    /* 变量说明：bit_size，位字段 bit 宽度。 */
    uint8_t bit_size;
} vm_comm_variable_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：type，对象类型或事件类型。 */
    vm_comm_event_type_t type;
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint32_t address;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint16_t size;
    /* 变量说明：protocol_command，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t protocol_command;
    /* 变量说明：error_code，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t error_code;
    /* 变量说明：has_numeric_value，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_numeric_value;
    /* 变量说明：numeric_value，保存当前对象运行所需的状态、参数或缓存数据。 */
    double numeric_value;
    /* 变量说明：name，变量名、页面名或节点名。 */
    char name[VM_COMM_NAME_MAX];
    /* 变量说明：display_value，保存当前对象运行所需的状态、参数或缓存数据。 */
    char display_value[VM_COMM_TEXT_MAX];
    /* 变量说明：message，保存当前对象运行所需的状态、参数或缓存数据。 */
    char message[VM_COMM_TEXT_MAX];
} vm_comm_event_t;

typedef void (*vm_comm_event_fn)(void *context, const vm_comm_event_t *event);

/**
 * 函数说明：vm_comm_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_comm_create(vm_comm_t **out);
/**
 * 函数说明：vm_comm_destroy，销毁对象并释放相关资源。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_comm_destroy(vm_comm_t *comm);
/**
 * 函数说明：vm_comm_set_device_config，执行本模块对应功能逻辑。
 * 输入：comm：通信模块上下文指针。；config：设备连接参数或模块初始化参数。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_comm_set_device_config(vm_comm_t *comm,
                                      const vm_comm_device_config_t *config);
/**
 * 函数说明：vm_comm_connect，建立连接并准备收发链路。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_comm_connect(vm_comm_t *comm);
/**
 * 函数说明：vm_comm_disconnect，建立连接并准备收发链路。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_comm_disconnect(vm_comm_t *comm);
/**
 * 函数说明：vm_comm_is_connected，建立连接并准备收发链路。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
uint8_t vm_comm_is_connected(const vm_comm_t *comm);
/**
 * 函数说明：vm_comm_set_event_callback，处理 UI 或通信事件。
 * 输入：comm：通信模块上下文指针。；callback：事件回调函数指针。；context：回调上下文指针，由调用方传入并在回调中原样返回。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_comm_set_event_callback(vm_comm_t *comm,
                                vm_comm_event_fn callback,
                                void *context);
/**
 * 函数说明：vm_comm_read_variable，读取数据或发起读取请求。
 * 输入：comm：通信模块上下文指针。；variable：变量描述信息，包含变量名、类型、地址、长度和权限。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_comm_read_variable(vm_comm_t *comm,
                                  const vm_comm_variable_t *variable);
/**
 * 函数说明：vm_comm_write_variable，写入数据或发起标定请求。
 * 输入：comm：通信模块上下文指针。；variable：变量描述信息，包含变量名、类型、地址、长度和权限。；target_text：UI 输入的目标值字符串，通常为十进制文本。；error：错误信息输出缓冲区。；error_size：错误信息缓冲区长度。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_comm_write_variable(vm_comm_t *comm,
                                   const vm_comm_variable_t *variable,
                                   const char *target_text,
                                   char *error,
                                   size_t error_size);
/**
 * 函数说明：vm_comm_poll，轮询处理异步收发事件。
 * 输入：comm：通信模块上下文指针。；budget：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_comm_poll(vm_comm_t *comm, uint8_t budget);

VM_END

#endif
