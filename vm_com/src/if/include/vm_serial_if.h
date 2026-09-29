/*
 * 文件说明：串口 IF 层接口，负责挂接当前平台串口设备模板并向上提供统一串口访问能力。
 * 所属模块：通信模块 / IF 层。
 * 设计要点：上层最多调用到 IF 层；MCAL 文件只提供设备模板，运行时设备创建、注册、注销、打开、关闭和读写由本层统一封装。
 */

#ifndef VM_SERIAL_IF_H
#define VM_SERIAL_IF_H

#include "vm_mcal_serial.h"

VM_BEGIN

/* 类型说明：串口 IF 控制块，内部保存当前平台模板、运行时设备链表和当前活动设备。 */
typedef struct vm_serial_if vm_serial_if_t;

/**
 * 函数说明：创建串口 IF 控制块。
 * 输入：out_if，输出控制块指针。
 * 输出：成功时 *out_if 指向已挂接当前平台串口模板的 IF 控制块。
 * 返回：VM_OK 表示成功，其它状态码表示参数错误、内存不足或当前平台不支持串口。
 */
vm_status_t vm_serial_if_create(vm_serial_if_t **out_if);

/**
 * 函数说明：销毁串口 IF 控制块。
 * 输入：serial_if，待销毁的 IF 控制块。
 * 输出：关闭并释放所有已注册设备，释放 IF 层内部节点。
 * 返回：无。
 */
void vm_serial_if_destroy(vm_serial_if_t *serial_if);

/**
 * 函数说明：按串口配置创建并注册一个运行时串口设备。
 * 输入：serial_if，IF 控制块；config，串口配置；out_device，输出设备对象。
 * 输出：成功时 *out_device 指向已注册设备。
 * 返回：VM_OK 表示成功，其它状态码表示配置非法、重复注册、内存不足或当前平台不支持。
 */
vm_status_t vm_serial_if_register_device(
    vm_serial_if_t *serial_if,
    const vm_mcal_serial_config_t *config,
    vm_mcal_serial_device_t **out_device);

/**
 * 函数说明：注销并释放一个运行时串口设备。
 * 输入：serial_if，IF 控制块；device，待注销设备。
 * 输出：设备从 IF 内部链表移除，若已打开则先关闭，然后释放设备对象。
 * 返回：VM_OK 表示成功，其它状态码表示未找到或参数错误。
 */
vm_status_t vm_serial_if_unregister_device(vm_serial_if_t *serial_if,
                                           vm_mcal_serial_device_t *device);

/**
 * 函数说明：打开 IF 层中已注册的串口设备。
 * 输入：serial_if，IF 控制块；device，待打开设备。
 * 输出：成功时 device 成为当前活动设备。
 * 返回：VM_OK 表示成功，其它状态码表示设备未注册、忙碌或底层打开失败。
 */
vm_status_t vm_serial_if_open(vm_serial_if_t *serial_if,
                              vm_mcal_serial_device_t *device);

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
