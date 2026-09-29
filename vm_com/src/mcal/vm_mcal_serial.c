/*
 * 文件说明：串口 MCAL 默认平台设备模板选择文件。
 * 所属模块：通信模块 / MCAL 层。
 * 设计要点：本文件类似板级设备选择文件，只向 IF 层返回当前编译平台的串口设备模板。
 */

#include "vm_mcal_serial_platform.h"

#ifdef _WIN32
/* 变量说明：Windows 平台串口设备模板，由 vm_windows_serial.c 实例化。 */
extern const vm_mcal_serial_device_t g_vm_windows_serial_device;
#else
/* 变量说明：POSIX 平台串口设备模板，由 vm_posix_serial.c 实例化。 */
extern const vm_mcal_serial_device_t g_vm_posix_serial_device;
#endif

const vm_mcal_serial_device_t *vm_mcal_serial_default_device_get(void)
{
#ifdef _WIN32
    return &g_vm_windows_serial_device;
#else
    return &g_vm_posix_serial_device;
#endif
}
