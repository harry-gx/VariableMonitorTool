/*
 * 文件说明：串口 IF 层接口，负责挂接平台串口驱动节点并向上提供统一串口访问能力。
 * 所属模块：通信模块 / IF 层。
 * 设计要点：上层最多调用到 IF 层；Windows/POSIX 等 MCAL 驱动作为独立节点注册到本层。
 */

#ifndef VM_SERIAL_IF_H
#define VM_SERIAL_IF_H

#include "vm_mcal_serial.h"

VM_BEGIN

/* 类型说明：串口 IF 控制块，内部保存已注册驱动节点、设备链表和当前活动设备。 */
typedef struct vm_serial_if vm_serial_if_t;

/**
 * 函数说明：创建串口 IF 控制块。
 * 输入：out_if，输出控制块指针。
 * 输出：成功时 *out_if 指向可注册驱动和设备的 IF 控制块。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误或内存不足。
 */
vm_status_t vm_serial_if_create(vm_serial_if_t **out_if);

/**
 * 函数说明：销毁串口 IF 控制块。
 * 输入：serial_if，待销毁的 IF 控制块。
 * 输出：关闭当前活动设备，并释放 IF 层保存的驱动链表节点。
 * 返回：无。
 */
void vm_serial_if_destroy(vm_serial_if_t *serial_if);

/**
 * 函数说明：注册一个 MCAL 串口驱动节点到 IF 层。
 * 输入：serial_if，IF 控制块；driver，平台驱动节点。
 * 输出：driver 加入 IF 内部驱动链表。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误、内存不足或重复注册。
 */
vm_status_t vm_serial_if_register_driver(
    vm_serial_if_t *serial_if,
    const vm_mcal_serial_driver_t *driver);

/**
 * 函数说明：注册本模块内置的默认串口驱动节点。
 * 输入：serial_if，IF 控制块。
 * 输出：Windows 和 POSIX 驱动节点被注册；运行时由 match 函数选择有效节点。
 * 返回：VM_OK 表示成功，其它状态码表示注册失败。
 */
vm_status_t vm_serial_if_register_default_drivers(vm_serial_if_t *serial_if);

/**
 * 函数说明：通过已注册驱动创建串口 MCAL 设备。
 * 输入：serial_if，IF 控制块；config，串口配置；out_device，输出设备对象。
 * 输出：成功时 *out_device 指向匹配平台驱动创建的设备对象。
 * 返回：VM_OK 表示成功，VM_UNSUPPORTED 表示没有驱动匹配，其它状态码表示创建失败。
 */
vm_status_t vm_serial_if_create_device(
    vm_serial_if_t *serial_if,
    const vm_mcal_serial_config_t *config,
    vm_mcal_serial_device_t **out_device);

/**
 * 函数说明：把一个 MCAL 串口设备注册到 IF 层。
 * 输入：serial_if，IF 控制块；device，待注册 MCAL 设备。
 * 输出：设备加入 IF 内部设备链表。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误或重复注册。
 */
vm_status_t vm_serial_if_register_device(vm_serial_if_t *serial_if,
                                         vm_mcal_serial_device_t *device);

/**
 * 函数说明：从 IF 层注销一个 MCAL 串口设备。
 * 输入：serial_if，IF 控制块；device，待注销 MCAL 设备。
 * 输出：设备从 IF 内部链表移除，如当前正在使用则先关闭。
 * 返回：VM_OK 表示成功，其它状态码表示未找到或参数错误。
 */
vm_status_t vm_serial_if_unregister_device(vm_serial_if_t *serial_if,
                                           vm_mcal_serial_device_t *device);

/**
 * 函数说明：按设备名查找已注册串口设备。
 * 输入：serial_if，IF 控制块；name，设备名称。
 * 输出：无。
 * 返回：找到时返回设备指针，未找到返回 NULL。
 */
vm_mcal_serial_device_t *vm_serial_if_find_device(vm_serial_if_t *serial_if,
                                                  const char *name);

/**
 * 函数说明：打开 IF 层中的指定串口设备。
 * 输入：serial_if，IF 控制块；name，设备名称；out_device，输出活动设备，可为 NULL。
 * 输出：成功时设备被打开并成为当前活动设备。
 * 返回：VM_OK 表示成功，其它状态码表示未找到、忙碌或打开失败。
 */
vm_status_t vm_serial_if_open(vm_serial_if_t *serial_if,
                              const char *name,
                              vm_mcal_serial_device_t **out_device);

/**
 * 函数说明：关闭 IF 层中的指定串口设备。
 * 输入：serial_if，IF 控制块；device，要关闭的设备，NULL 表示当前活动设备。
 * 输出：底层串口关闭，当前活动设备按需清空。
 * 返回：无。
 */
void vm_serial_if_close(vm_serial_if_t *serial_if,
                        vm_mcal_serial_device_t *device);

/**
 * 函数说明：查询串口设备是否处于打开状态。
 * 输入：serial_if，IF 控制块；device，待查询设备，NULL 表示当前活动设备。
 * 输出：无。
 * 返回：1 表示已打开，0 表示未打开或参数无效。
 */
uint8_t vm_serial_if_is_open(const vm_serial_if_t *serial_if,
                             const vm_mcal_serial_device_t *device);

/**
 * 函数说明：从 IF 层读取串口数据。
 * 输入：serial_if，IF 控制块；device，读取设备，NULL 表示当前活动设备；buffer，接收缓冲区；capacity，缓冲区容量。
 * 输出：read_size 返回实际读取字节数。
 * 返回：VM_OK 表示读到数据，VM_NOT_FOUND 表示暂无数据，其它状态码表示错误。
 */
vm_status_t vm_serial_if_read(vm_serial_if_t *serial_if,
                              vm_mcal_serial_device_t *device,
                              uint8_t *buffer,
                              size_t capacity,
                              size_t *read_size);

/**
 * 函数说明：通过 IF 层发送串口数据。
 * 输入：serial_if，IF 控制块；device，发送设备，NULL 表示当前活动设备；data，待发送数据；size，待发送长度。
 * 输出：数据被交给 MCAL 层发送。
 * 返回：VM_OK 表示成功，其它状态码表示错误。
 */
vm_status_t vm_serial_if_write(vm_serial_if_t *serial_if,
                               vm_mcal_serial_device_t *device,
                               const uint8_t *data,
                               size_t size);

VM_END

#endif
