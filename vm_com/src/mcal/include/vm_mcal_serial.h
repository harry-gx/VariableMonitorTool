/*
 * 文件说明：串口 MCAL 层统一接口，定义串口驱动节点、设备对象和底层读写操作表。
 * 所属模块：通信模块 / MCAL 层。
 * 设计要点：Windows、POSIX 等平台串口驱动各自实现独立 driver node，由 Serial IF 层注册和选择。
 */

#ifndef VM_MCAL_SERIAL_H
#define VM_MCAL_SERIAL_H

#include "vm_list.h"
#include "vm_status.h"

VM_BEGIN

#define VM_MCAL_SERIAL_NAME_MAX (128u)

typedef enum
{
    VM_MCAL_SERIAL_PARITY_NONE = 0,
    VM_MCAL_SERIAL_PARITY_ODD = 1,
    VM_MCAL_SERIAL_PARITY_EVEN = 2
} vm_mcal_serial_parity_t;

typedef enum
{
    VM_MCAL_SERIAL_FLOW_NONE = 0,
    VM_MCAL_SERIAL_FLOW_RTS_CTS = 1,
    VM_MCAL_SERIAL_FLOW_XON_XOFF = 2
} vm_mcal_serial_flow_t;

typedef struct
{
    const char *device_name;
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t flow_control;
} vm_mcal_serial_config_t;

typedef struct
{
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_error_count;
    uint64_t tx_error_count;
    uint64_t overflow_count;
} vm_mcal_serial_stats_t;

typedef struct vm_mcal_serial_device vm_mcal_serial_device_t;
typedef struct vm_mcal_serial_driver vm_mcal_serial_driver_t;

typedef vm_status_t (*vm_mcal_serial_open_fn)(
    vm_mcal_serial_device_t *device);
typedef vm_status_t (*vm_mcal_serial_close_fn)(
    vm_mcal_serial_device_t *device);
typedef vm_status_t (*vm_mcal_serial_read_fn)(
    vm_mcal_serial_device_t *device,
    uint8_t *buffer,
    size_t capacity,
    size_t *read_size);
typedef vm_status_t (*vm_mcal_serial_write_fn)(
    vm_mcal_serial_device_t *device,
    const uint8_t *data,
    size_t size);
typedef vm_status_t (*vm_mcal_serial_param_set_fn)(
    vm_mcal_serial_device_t *device,
    const vm_mcal_serial_config_t *config);
typedef vm_status_t (*vm_mcal_serial_param_get_fn)(
    const vm_mcal_serial_device_t *device,
    vm_mcal_serial_config_t *config);

typedef struct
{
    vm_mcal_serial_open_fn open;
    vm_mcal_serial_close_fn close;
    vm_mcal_serial_read_fn read;
    vm_mcal_serial_write_fn write;
    vm_mcal_serial_param_set_fn param_set;
    vm_mcal_serial_param_get_fn param_get;
} vm_mcal_serial_ops_t;

typedef uint8_t (*vm_mcal_serial_driver_match_fn)(
    const vm_mcal_serial_config_t *config);
typedef vm_status_t (*vm_mcal_serial_driver_create_fn)(
    const vm_mcal_serial_config_t *config,
    vm_mcal_serial_device_t **out_device);

struct vm_mcal_serial_driver
{
    const char *driver_name;
    vm_mcal_serial_driver_match_fn match;
    vm_mcal_serial_driver_create_fn create;
};

struct vm_mcal_serial_device
{
    list_head_t node;
    char name[VM_MCAL_SERIAL_NAME_MAX];
    vm_mcal_serial_config_t config;
    const vm_mcal_serial_driver_t *driver;
    const vm_mcal_serial_ops_t *ops;
    void *driver_context;
    vm_mcal_serial_stats_t stats;
    uint8_t opened;
};

/**
 * 函数说明：校验串口配置是否合法。
 * 输入：config，待校验的串口配置。
 * 输出：无。
 * 返回：VM_OK 表示合法，其它状态码表示配置缺失或取值不支持。
 */
vm_status_t vm_mcal_serial_config_validate(
    const vm_mcal_serial_config_t *config);

/**
 * 函数说明：初始化一个由具体平台驱动分配的串口设备对象。
 * 输入：device，设备对象；driver，所属驱动节点；config，串口配置；ops，设备操作表；driver_context，平台私有上下文。
 * 输出：device 填入名称、配置、驱动节点、操作表和私有上下文。
 * 返回：VM_OK 表示成功，其它状态码表示参数非法。
 */
vm_status_t vm_mcal_serial_device_configure(
    vm_mcal_serial_device_t *device,
    const vm_mcal_serial_driver_t *driver,
    const vm_mcal_serial_config_t *config,
    const vm_mcal_serial_ops_t *ops,
    void *driver_context);

/**
 * 函数说明：销毁串口 MCAL 设备。
 * 输入：device，待销毁的设备对象。
 * 输出：设备会先关闭，再释放平台上下文和设备对象。
 * 返回：无。
 */
void vm_mcal_serial_destroy(vm_mcal_serial_device_t *device);

vm_status_t vm_mcal_serial_open(vm_mcal_serial_device_t *device);
vm_status_t vm_mcal_serial_close(vm_mcal_serial_device_t *device);
vm_status_t vm_mcal_serial_read(vm_mcal_serial_device_t *device,
                                uint8_t *buffer,
                                size_t capacity,
                                size_t *read_size);
vm_status_t vm_mcal_serial_write(vm_mcal_serial_device_t *device,
                                 const uint8_t *data,
                                 size_t size);

/**
 * 函数说明：获取 Windows 串口驱动节点。
 * 输入：无。
 * 输出：无。
 * 返回：Windows 平台驱动节点；非 Windows 构建中该节点不匹配任何设备。
 */
const vm_mcal_serial_driver_t *vm_mcal_serial_windows_driver_get(void);

/**
 * 函数说明：获取 POSIX 串口驱动节点。
 * 输入：无。
 * 输出：无。
 * 返回：POSIX 平台驱动节点；Windows 构建中该节点不匹配任何设备。
 */
const vm_mcal_serial_driver_t *vm_mcal_serial_posix_driver_get(void);

VM_END

#endif
