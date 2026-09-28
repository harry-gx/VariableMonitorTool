/*
 * 文件说明：串口 MCAL 公共实现，提供配置校验、设备初始化和统一 open/read/write 包装。
 * 所属模块：通信模块 / SHAL MCAL 层。
 * 设计要点：本文件不包含任何平台 API，平台差异全部放在 boards 下的独立驱动节点中。
 */

#include "vm_mcal_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static vm_status_t vm_mcal_serial_copy_name(vm_mcal_serial_device_t *device,
                                            const char *name)
{
    vm_status_t status;

    status = VM_INVALID;
    if ((device != NULL) && (name != NULL) && (name[0] != '\0'))
    {
        (void)snprintf(device->name, sizeof(device->name), "%s", name);
        device->name[sizeof(device->name) - 1u] = '\0';
        device->config.device_name = device->name;
        status = VM_OK;
    }

    return status;
}

vm_status_t vm_mcal_serial_config_validate(
    const vm_mcal_serial_config_t *config)
{
    vm_status_t status;

    status = VM_OK;
    if ((config == NULL) || (config->device_name == NULL) ||
        (config->device_name[0] == '\0') || (config->baudrate == 0u))
    {
        status = VM_INVALID;
    }
    else if ((config->data_bits < 5u) || (config->data_bits > 8u) ||
             ((config->stop_bits != 1u) && (config->stop_bits != 2u)) ||
             (config->parity > (uint8_t)VM_MCAL_SERIAL_PARITY_EVEN) ||
             (config->flow_control > (uint8_t)VM_MCAL_SERIAL_FLOW_XON_XOFF))
    {
        status = VM_FORMAT;
    }
    else
    {
        /* 配置合法，无需额外处理。 */
    }

    return status;
}

vm_status_t vm_mcal_serial_device_configure(
    vm_mcal_serial_device_t *device,
    const vm_mcal_serial_driver_t *driver,
    const vm_mcal_serial_config_t *config,
    const vm_mcal_serial_ops_t *ops,
    void *driver_context)
{
    vm_status_t status;

    if ((device == NULL) || (driver == NULL) || (ops == NULL) ||
        (driver_context == NULL) || (ops->open == NULL) ||
        (ops->close == NULL) || (ops->read == NULL) ||
        (ops->write == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = vm_mcal_serial_config_validate(config);
        if (status == VM_OK)
        {
            (void)memset(device, 0, sizeof(*device));
            INIT_LIST_HEAD(&device->node);
            device->config = *config;
            device->driver = driver;
            device->ops = ops;
            device->driver_context = driver_context;
            status = vm_mcal_serial_copy_name(device, config->device_name);
        }
    }

    return status;
}

void vm_mcal_serial_destroy(vm_mcal_serial_device_t *device)
{
    if (device != NULL)
    {
        (void)vm_mcal_serial_close(device);
        free(device->driver_context);
        device->driver_context = NULL;
        free(device);
    }
}

vm_status_t vm_mcal_serial_open(vm_mcal_serial_device_t *device)
{
    vm_status_t status;

    if ((device == NULL) || (device->ops == NULL) ||
        (device->ops->open == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = device->ops->open(device);
    }

    return status;
}

vm_status_t vm_mcal_serial_close(vm_mcal_serial_device_t *device)
{
    vm_status_t status;

    if (device == NULL)
    {
        status = VM_OK;
    }
    else if ((device->ops == NULL) || (device->ops->close == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = device->ops->close(device);
    }

    return status;
}

vm_status_t vm_mcal_serial_read(vm_mcal_serial_device_t *device,
                                uint8_t *buffer,
                                size_t capacity,
                                size_t *read_size)
{
    vm_status_t status;

    if (read_size != NULL)
    {
        *read_size = 0u;
    }

    if ((device == NULL) || (buffer == NULL) || (capacity == 0u) ||
        (read_size == NULL) || (device->ops == NULL) ||
        (device->ops->read == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = device->ops->read(device, buffer, capacity, read_size);
    }

    return status;
}

vm_status_t vm_mcal_serial_write(vm_mcal_serial_device_t *device,
                                 const uint8_t *data,
                                 size_t size)
{
    vm_status_t status;

    if ((device == NULL) || ((data == NULL) && (size > 0u)) ||
        (device->ops == NULL) || (device->ops->write == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = device->ops->write(device, data, size);
    }

    return status;
}
