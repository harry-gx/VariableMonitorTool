/*
 * 文件说明：串口 MCAL 平台设备模板查询接口。
 * 所属模块：通信模块 / MCAL 层。
 * 设计要点：IF 层通过本接口获取当前编译平台的默认串口设备模板，不直接依赖具体平台源文件。
 */

#ifndef VM_MCAL_SERIAL_PLATFORM_H
#define VM_MCAL_SERIAL_PLATFORM_H

#include "vm_mcal_serial.h"

VM_BEGIN

/**
 * 函数说明：获取当前编译平台的默认串口设备模板。
 * 输入：无。
 * 输出：无。
 * 返回：当前平台设备模板指针；当前构建不支持串口时返回 NULL。
 */
const vm_mcal_serial_device_t *vm_mcal_serial_default_device_get(void);

VM_END

#endif
