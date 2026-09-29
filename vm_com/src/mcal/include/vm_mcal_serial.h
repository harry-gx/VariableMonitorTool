/*
 * 文件说明：串口 MCAL 层设备描述头文件。
 * 所属模块：通信模块 / MCAL 层。
 * 设计要点：本文件只提供串口驱动所需的配置结构、设备结构、统计结构和操作函数指针。
 *           Windows、POSIX 等平台文件只实例化 vm_mcal_serial_device_t 模板；平台选择和设备创建由 IF 层完成。
 */

#ifndef VM_MCAL_SERIAL_H
#define VM_MCAL_SERIAL_H

#include "vm_list.h"
#include "vm_status.h"

VM_BEGIN

/* 宏说明：串口设备名称缓存长度，保存 COMx、/dev/ttyUSBx 等设备名。 */
#define VM_MCAL_SERIAL_NAME_MAX (128u)

/* 类型说明：串口校验位配置。 */
typedef enum
{
    VM_MCAL_SERIAL_PARITY_NONE = 0,
    VM_MCAL_SERIAL_PARITY_ODD = 1,
    VM_MCAL_SERIAL_PARITY_EVEN = 2
} vm_mcal_serial_parity_t;

/* 类型说明：串口流控配置。 */
typedef enum
{
    VM_MCAL_SERIAL_FLOW_NONE = 0,
    VM_MCAL_SERIAL_FLOW_RTS_CTS = 1,
    VM_MCAL_SERIAL_FLOW_XON_XOFF = 2
} vm_mcal_serial_flow_t;


/*
 * 类型说明：串口驱动配置。
 * 成员说明：
 * device_name  串口设备名称缓存，例如 COM15 或 /dev/ttyUSB0。
 * baudrate     串口波特率，单位 bit/s。
 * data_bits    数据位，支持范围由 IF 层统一校验。
 * stop_bits    停止位，支持 1 或 2。
 * parity       校验位，取值参考 vm_mcal_serial_parity_t。
 * flow_control 流控模式，取值参考 vm_mcal_serial_flow_t。
 */
typedef struct
{
    char device_name[VM_MCAL_SERIAL_NAME_MAX];
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t flow_control;
} vm_mcal_serial_config_t;

/* 类型说明：串口设备收发统计，供上层诊断链路质量。 */
typedef struct
{
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_error_count;
    uint64_t tx_error_count;
    uint64_t overflow_count;
} vm_mcal_serial_stats_t;

typedef struct vm_mcal_serial_device vm_mcal_serial_device_t;

/* 函数指针说明：打开平台串口设备。 */
typedef vm_status_t (*vm_mcal_serial_open_fn)(
    vm_mcal_serial_device_t *device);

/* 函数指针说明：关闭平台串口设备。 */
typedef vm_status_t (*vm_mcal_serial_close_fn)(
    vm_mcal_serial_device_t *device);

/* 函数指针说明：从平台串口设备读取数据。 */
typedef vm_status_t (*vm_mcal_serial_read_fn)(
    vm_mcal_serial_device_t *device,
    uint8_t *buffer,
    size_t capacity,
    size_t *read_size);

/* 函数指针说明：向平台串口设备写入数据。 */
typedef vm_status_t (*vm_mcal_serial_write_fn)(
    vm_mcal_serial_device_t *device,
    const uint8_t *data,
    size_t size);


/* 类型说明：串口设备操作表，由具体平台驱动文件实例化。 */
typedef struct
{
    vm_mcal_serial_open_fn open;
    vm_mcal_serial_close_fn close;
    vm_mcal_serial_read_fn read;
    vm_mcal_serial_write_fn write;
} vm_mcal_serial_ops_t;

/*
 * 类型说明：串口 MCAL 设备对象。
 * 成员说明：
 * node                侵入式链表节点，由 IF 层用于挂接平台模板或运行时设备。
 * platform_name       平台设备模板名称，例如 windows_serial 或 posix_serial。
 * config              运行时串口配置；config.device_name 是唯一保存的串口设备名。
 * ops                 平台操作表，平台文件实例化模板时提供。
 * platform_context_size 平台私有上下文大小，IF 层据此分配上下文内存。
 * platform_context      平台私有上下文指针，实际设备对象创建后由 IF 层分配。
 * stats               收发统计。
 * opened              打开状态。
 */
struct vm_mcal_serial_device
{
    list_head_t node;
    const char *platform_name;
    vm_mcal_serial_config_t config;
    const vm_mcal_serial_ops_t *ops;
    size_t platform_context_size;
    void *platform_context;
    vm_mcal_serial_stats_t stats;
    uint8_t opened;
};

VM_END

#endif
