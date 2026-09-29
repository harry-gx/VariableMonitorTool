/*
 * 文件说明：串口 IF 层实现，统一管理当前平台串口设备模板和运行时 MCAL 设备对象。
 * 所属模块：通信模块 / IF 层。
 * 设计要点：IF 层只维护一个运行时设备链表，当前平台模板用指针保存，不单独挂链表。
 */

#include "vm_serial_if.h"

#include "vm_list.h"
#include "vm_mcal_serial_platform.h"

#include <stdlib.h>
#include <string.h>

struct vm_serial_if
{
    const vm_mcal_serial_device_t *platform_device;
    list_head_t device_list;
    vm_mcal_serial_device_t *active_device;
};

static uint8_t vm_serial_if_name_equal(const char *left, const char *right)
{
    uint8_t equal;

    equal = 0u;
    if ((left != NULL) && (right != NULL) && (strcmp(left, right) == 0))
    {
        equal = 1u;
    }

    return equal;
}

static vm_status_t vm_serial_if_config_validate(
    const vm_mcal_serial_config_t *config)
{
    vm_status_t status;

    status = VM_OK;
    if ((config == NULL) || (config->device_name[0] == '\0') ||
        (config->baudrate == 0u))
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

static uint8_t vm_serial_if_device_ops_valid(
    const vm_mcal_serial_device_t *device)
{
    uint8_t valid;

    valid = 0u;
    if ((device != NULL) && (device->ops != NULL) &&
        (device->ops->open != NULL) && (device->ops->close != NULL) &&
        (device->ops->read != NULL) && (device->ops->write != NULL))
    {
        valid = 1u;
    }

    return valid;
}

static uint8_t vm_serial_if_device_registered(
    const vm_serial_if_t *serial_if,
    const vm_mcal_serial_device_t *device)
{
    const list_head_t *node;
    const vm_mcal_serial_device_t *current;
    uint8_t registered;

    registered = 0u;
    if ((serial_if != NULL) && (device != NULL))
    {
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            current = list_entry(node, vm_mcal_serial_device_t, node);
            if (current == device)
            {
                registered = 1u;
                break;
            }
            node = node->next;
        }
    }

    return registered;
}

static uint8_t vm_serial_if_device_name_exists(
    const vm_serial_if_t *serial_if,
    const char *device_name)
{
    const list_head_t *node;
    const vm_mcal_serial_device_t *current;
    uint8_t exists;

    exists = 0u;
    if ((serial_if != NULL) && (device_name != NULL))
    {
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            current = list_entry(node, vm_mcal_serial_device_t, node);
            if (vm_serial_if_name_equal(current->config.device_name,
                                        device_name) != 0u)
            {
                exists = 1u;
                break;
            }
            node = node->next;
        }
    }

    return exists;
}

static vm_status_t vm_serial_if_device_open(vm_mcal_serial_device_t *device)
{
    vm_status_t status;

    if (vm_serial_if_device_ops_valid(device) == 0u)
    {
        status = VM_INVALID;
    }
    else
    {
        status = device->ops->open(device);
    }

    return status;
}

static vm_status_t vm_serial_if_device_close(vm_mcal_serial_device_t *device)
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

static void vm_serial_if_device_destroy(vm_mcal_serial_device_t *device)
{
    if (device != NULL)
    {
        (void)vm_serial_if_device_close(device);
        free(device->platform_context);
        device->platform_context = NULL;
        free(device);
    }
}

static void vm_serial_if_free_devices(vm_serial_if_t *serial_if)
{
    list_head_t *node;
    list_head_t *next_node;
    vm_mcal_serial_device_t *device;

    if (serial_if != NULL)
    {
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            next_node = node->next;
            device = list_entry(node, vm_mcal_serial_device_t, node);
            LIST_DEL(&device->node);
            INIT_LIST_HEAD(&device->node);
            vm_serial_if_device_destroy(device);
            node = next_node;
        }
        serial_if->active_device = NULL;
    }
}

static vm_status_t vm_serial_if_apply_config(
    vm_mcal_serial_device_t *device,
    const vm_mcal_serial_device_t *platform_device,
    const vm_mcal_serial_config_t *config)
{
    vm_status_t status;

    if ((device == NULL) || (platform_device == NULL) || (config == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        (void)memset(device, 0, sizeof(*device));
        INIT_LIST_HEAD(&device->node);
        device->platform_name = platform_device->platform_name;
        device->ops = platform_device->ops;
        device->platform_context_size = platform_device->platform_context_size;
        device->config = *config;
        device->config.device_name[sizeof(device->config.device_name) - 1u] = '\0';
        status = VM_OK;
    }

    return status;
}

static vm_status_t vm_serial_if_create_from_platform(
    const vm_mcal_serial_device_t *platform_device,
    const vm_mcal_serial_config_t *config,
    vm_mcal_serial_device_t **out_device)
{
    vm_mcal_serial_device_t *device;
    void *context;
    vm_status_t status;

    if ((platform_device == NULL) || (config == NULL) ||
        (out_device == NULL) || (platform_device->platform_context_size == 0u) ||
        (vm_serial_if_device_ops_valid(platform_device) == 0u))
    {
        status = VM_INVALID;
    }
    else
    {
        *out_device = NULL;
        device = (vm_mcal_serial_device_t *)calloc(1u, sizeof(*device));
        context = calloc(1u, platform_device->platform_context_size);
        if ((device == NULL) || (context == NULL))
        {
            free(device);
            free(context);
            status = VM_NOMEM;
        }
        else
        {
            status = vm_serial_if_apply_config(device, platform_device, config);
            if (status == VM_OK)
            {
                device->platform_context = context;
                *out_device = device;
            }
            else
            {
                free(context);
                free(device);
            }
        }
    }

    return status;
}

vm_status_t vm_serial_if_create(vm_serial_if_t **out_if)
{
    vm_serial_if_t *serial_if;
    const vm_mcal_serial_device_t *platform_device;
    vm_status_t status;

    if (out_if == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out_if = NULL;
        platform_device = vm_mcal_serial_default_device_get();
        if (platform_device == NULL)
        {
            status = VM_UNSUPPORTED;
        }
        else
        {
            serial_if = (vm_serial_if_t *)calloc(1u, sizeof(*serial_if));
            if (serial_if == NULL)
            {
                status = VM_NOMEM;
            }
            else
            {
                serial_if->platform_device = platform_device;
                INIT_LIST_HEAD(&serial_if->device_list);
                *out_if = serial_if;
                status = VM_OK;
            }
        }
    }

    return status;
}

void vm_serial_if_destroy(vm_serial_if_t *serial_if)
{
    if (serial_if != NULL)
    {
        vm_serial_if_close(serial_if, NULL);
        vm_serial_if_free_devices(serial_if);
        free(serial_if);
    }
}

vm_status_t vm_serial_if_register_device(
    vm_serial_if_t *serial_if,
    const vm_mcal_serial_config_t *config,
    vm_mcal_serial_device_t **out_device)
{
    vm_mcal_serial_device_t *device;
    vm_status_t status;

    if (out_device == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out_device = NULL;
        status = vm_serial_if_config_validate(config);
        if ((serial_if == NULL) || (status != VM_OK))
        {
            if (serial_if == NULL)
            {
                status = VM_INVALID;
            }
        }
        else if (vm_serial_if_device_name_exists(serial_if,
                                                 config->device_name) != 0u)
        {
            status = VM_DUPLICATE;
        }
        else if (serial_if->platform_device == NULL)
        {
            status = VM_UNSUPPORTED;
        }
        else
        {
            device = NULL;
            status = vm_serial_if_create_from_platform(serial_if->platform_device,
                                                       config,
                                                       &device);
            if (status == VM_OK)
            {
                LIST_ADD_TAIL(&device->node, &serial_if->device_list);
                *out_device = device;
            }
        }
    }

    return status;
}

vm_status_t vm_serial_if_unregister_device(vm_serial_if_t *serial_if,
                                           vm_mcal_serial_device_t *device)
{
    vm_status_t status;

    if ((serial_if == NULL) || (device == NULL))
    {
        status = VM_INVALID;
    }
    else if (vm_serial_if_device_registered(serial_if, device) == 0u)
    {
        status = VM_NOT_FOUND;
    }
    else
    {
        if (serial_if->active_device == device)
        {
            vm_serial_if_close(serial_if, device);
        }
        LIST_DEL(&device->node);
        INIT_LIST_HEAD(&device->node);
        vm_serial_if_device_destroy(device);
        status = VM_OK;
    }

    return status;
}

vm_status_t vm_serial_if_open(vm_serial_if_t *serial_if,
                              vm_mcal_serial_device_t *device)
{
    vm_status_t status;

    if ((serial_if == NULL) || (device == NULL))
    {
        status = VM_INVALID;
    }
    else if (vm_serial_if_device_registered(serial_if, device) == 0u)
    {
        status = VM_NOT_FOUND;
    }
    else
    {
        if ((serial_if->active_device != NULL) &&
            (serial_if->active_device != device))
        {
            vm_serial_if_close(serial_if, serial_if->active_device);
        }
        status = vm_serial_if_device_open(device);
        if (status == VM_OK)
        {
            serial_if->active_device = device;
        }
    }

    return status;
}

void vm_serial_if_close(vm_serial_if_t *serial_if,
                        vm_mcal_serial_device_t *device)
{
    vm_mcal_serial_device_t *target;

    if (serial_if != NULL)
    {
        target = device;
        if (target == NULL)
        {
            target = serial_if->active_device;
        }

        if (target != NULL)
        {
            (void)vm_serial_if_device_close(target);
            if (serial_if->active_device == target)
            {
                serial_if->active_device = NULL;
            }
        }
    }
}

uint8_t vm_serial_if_is_open(const vm_serial_if_t *serial_if,
                             const vm_mcal_serial_device_t *device)
{
    const vm_mcal_serial_device_t *target;
    uint8_t opened;

    opened = 0u;
    if (serial_if != NULL)
    {
        target = device;
        if (target == NULL)
        {
            target = serial_if->active_device;
        }
        if ((target != NULL) && (target->opened != 0u))
        {
            opened = 1u;
        }
    }

    return opened;
}

vm_status_t vm_serial_if_read(vm_serial_if_t *serial_if,
                              vm_mcal_serial_device_t *device,
                              uint8_t *buffer,
                              size_t capacity,
                              size_t *read_size)
{
    vm_mcal_serial_device_t *target;
    vm_status_t status;

    if (read_size != NULL)
    {
        *read_size = 0u;
    }

    if (serial_if == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        target = device;
        if (target == NULL)
        {
            target = serial_if->active_device;
        }
        if (target == NULL)
        {
            status = VM_BUSY;
        }
        else if ((buffer == NULL) || (capacity == 0u) ||
                 (read_size == NULL) || (target->ops == NULL) ||
                 (target->ops->read == NULL))
        {
            status = VM_INVALID;
        }
        else
        {
            status = target->ops->read(target, buffer, capacity, read_size);
        }
    }

    return status;
}

vm_status_t vm_serial_if_write(vm_serial_if_t *serial_if,
                               vm_mcal_serial_device_t *device,
                               const uint8_t *data,
                               size_t size)
{
    vm_mcal_serial_device_t *target;
    vm_status_t status;

    if (serial_if == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        target = device;
        if (target == NULL)
        {
            target = serial_if->active_device;
        }
        if (target == NULL)
        {
            status = VM_BUSY;
        }
        else if (((data == NULL) && (size > 0u)) ||
                 (target->ops == NULL) || (target->ops->write == NULL))
        {
            status = VM_INVALID;
        }
        else
        {
            status = target->ops->write(target, data, size);
        }
    }

    return status;
}
