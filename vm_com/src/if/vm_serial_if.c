/*
 * 文件说明：串口 IF 层实现，统一管理平台串口驱动节点和 MCAL 设备对象。
 * 所属模块：通信模块 / IF 层。
 * 设计要点：驱动挂接节点和设备节点均使用通信模块内拷贝的 vm_list.h 侵入式链表。
 */

#include "vm_serial_if.h"

#include "vm_list.h"

#include <stdlib.h>
#include <string.h>

typedef struct
{
    list_head_t node;
    const vm_mcal_serial_driver_t *driver;
} vm_serial_if_driver_node_t;

struct vm_serial_if
{
    list_head_t driver_list;
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

static uint8_t vm_serial_if_driver_exists(
    vm_serial_if_t *serial_if,
    const vm_mcal_serial_driver_t *driver)
{
    list_head_t *node;
    vm_serial_if_driver_node_t *entry;
    uint8_t exists;

    exists = 0u;
    if ((serial_if != NULL) && (driver != NULL))
    {
        node = serial_if->driver_list.next;
        while (node != &serial_if->driver_list)
        {
            entry = list_entry(node, vm_serial_if_driver_node_t, node);
            if ((entry->driver == driver) ||
                (vm_serial_if_name_equal(entry->driver->driver_name,
                                         driver->driver_name) != 0u))
            {
                exists = 1u;
                break;
            }
            node = node->next;
        }
    }

    return exists;
}

static void vm_serial_if_free_driver_nodes(vm_serial_if_t *serial_if)
{
    list_head_t *node;
    list_head_t *next_node;
    vm_serial_if_driver_node_t *entry;

    if (serial_if != NULL)
    {
        node = serial_if->driver_list.next;
        while (node != &serial_if->driver_list)
        {
            next_node = node->next;
            entry = list_entry(node, vm_serial_if_driver_node_t, node);
            LIST_DEL(&entry->node);
            free(entry);
            node = next_node;
        }
    }
}

static void vm_serial_if_detach_devices(vm_serial_if_t *serial_if)
{
    list_head_t *node;
    list_head_t *next_node;

    if (serial_if != NULL)
    {
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            next_node = node->next;
            LIST_DEL(node);
            INIT_LIST_HEAD(node);
            node = next_node;
        }
        serial_if->active_device = NULL;
    }
}

vm_status_t vm_serial_if_create(vm_serial_if_t **out_if)
{
    vm_serial_if_t *serial_if;
    vm_status_t status;

    if (out_if == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out_if = NULL;
        serial_if = (vm_serial_if_t *)calloc(1u, sizeof(*serial_if));
        if (serial_if == NULL)
        {
            status = VM_NOMEM;
        }
        else
        {
            INIT_LIST_HEAD(&serial_if->driver_list);
            INIT_LIST_HEAD(&serial_if->device_list);
            *out_if = serial_if;
            status = VM_OK;
        }
    }

    return status;
}

void vm_serial_if_destroy(vm_serial_if_t *serial_if)
{
    if (serial_if != NULL)
    {
        vm_serial_if_close(serial_if, NULL);
        vm_serial_if_detach_devices(serial_if);
        vm_serial_if_free_driver_nodes(serial_if);
        free(serial_if);
    }
}

vm_status_t vm_serial_if_register_driver(
    vm_serial_if_t *serial_if,
    const vm_mcal_serial_driver_t *driver)
{
    vm_serial_if_driver_node_t *entry;
    vm_status_t status;

    if ((serial_if == NULL) || (driver == NULL) ||
        (driver->driver_name == NULL) || (driver->match == NULL) ||
        (driver->create == NULL))
    {
        status = VM_INVALID;
    }
    else if (vm_serial_if_driver_exists(serial_if, driver) != 0u)
    {
        status = VM_DUPLICATE;
    }
    else
    {
        entry = (vm_serial_if_driver_node_t *)calloc(1u, sizeof(*entry));
        if (entry == NULL)
        {
            status = VM_NOMEM;
        }
        else
        {
            INIT_LIST_HEAD(&entry->node);
            entry->driver = driver;
            LIST_ADD_TAIL(&entry->node, &serial_if->driver_list);
            status = VM_OK;
        }
    }

    return status;
}

vm_status_t vm_serial_if_register_default_drivers(vm_serial_if_t *serial_if)
{
    vm_status_t status;

    status = vm_serial_if_register_driver(
        serial_if,
        vm_mcal_serial_windows_driver_get());
    if (status == VM_OK)
    {
        status = vm_serial_if_register_driver(
            serial_if,
            vm_mcal_serial_posix_driver_get());
    }

    return status;
}

vm_status_t vm_serial_if_create_device(
    vm_serial_if_t *serial_if,
    const vm_mcal_serial_config_t *config,
    vm_mcal_serial_device_t **out_device)
{
    list_head_t *node;
    vm_serial_if_driver_node_t *entry;
    vm_status_t status;

    if (out_device == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out_device = NULL;
        if ((serial_if == NULL) || (config == NULL))
        {
            status = VM_INVALID;
        }
        else
        {
            status = VM_UNSUPPORTED;
            node = serial_if->driver_list.next;
            while (node != &serial_if->driver_list)
            {
                entry = list_entry(node, vm_serial_if_driver_node_t, node);
                if (entry->driver->match(config) != 0u)
                {
                    status = entry->driver->create(config, out_device);
                    break;
                }
                node = node->next;
            }
        }
    }

    return status;
}

vm_status_t vm_serial_if_register_device(vm_serial_if_t *serial_if,
                                         vm_mcal_serial_device_t *device)
{
    list_head_t *node;
    vm_mcal_serial_device_t *current;
    vm_status_t status;

    if ((serial_if == NULL) || (device == NULL) || (device->name[0] == '\0'))
    {
        status = VM_INVALID;
    }
    else
    {
        status = VM_OK;
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            current = list_entry(node, vm_mcal_serial_device_t, node);
            if ((current == device) ||
                (vm_serial_if_name_equal(current->name, device->name) != 0u))
            {
                status = VM_DUPLICATE;
                break;
            }
            node = node->next;
        }

        if (status == VM_OK)
        {
            LIST_ADD_TAIL(&device->node, &serial_if->device_list);
        }
    }

    return status;
}

vm_status_t vm_serial_if_unregister_device(vm_serial_if_t *serial_if,
                                           vm_mcal_serial_device_t *device)
{
    list_head_t *node;
    vm_mcal_serial_device_t *current;
    vm_status_t status;

    if ((serial_if == NULL) || (device == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = VM_NOT_FOUND;
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            current = list_entry(node, vm_mcal_serial_device_t, node);
            if (current == device)
            {
                if (serial_if->active_device == current)
                {
                    vm_serial_if_close(serial_if, current);
                }
                LIST_DEL(&current->node);
                INIT_LIST_HEAD(&current->node);
                status = VM_OK;
                break;
            }
            node = node->next;
        }
    }

    return status;
}

vm_mcal_serial_device_t *vm_serial_if_find_device(vm_serial_if_t *serial_if,
                                                  const char *name)
{
    list_head_t *node;
    vm_mcal_serial_device_t *current;
    vm_mcal_serial_device_t *found;

    found = NULL;
    if ((serial_if != NULL) && (name != NULL))
    {
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            current = list_entry(node, vm_mcal_serial_device_t, node);
            if (vm_serial_if_name_equal(current->name, name) != 0u)
            {
                found = current;
                break;
            }
            node = node->next;
        }
    }

    return found;
}

vm_status_t vm_serial_if_open(vm_serial_if_t *serial_if,
                              const char *name,
                              vm_mcal_serial_device_t **out_device)
{
    vm_mcal_serial_device_t *device;
    vm_status_t status;

    if (out_device != NULL)
    {
        *out_device = NULL;
    }

    if ((serial_if == NULL) || (name == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        device = vm_serial_if_find_device(serial_if, name);
        if (device == NULL)
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
            status = vm_mcal_serial_open(device);
            if (status == VM_OK)
            {
                serial_if->active_device = device;
                if (out_device != NULL)
                {
                    *out_device = device;
                }
            }
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
            (void)vm_mcal_serial_close(target);
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
        else
        {
            status = vm_mcal_serial_read(target,
                                         buffer,
                                         capacity,
                                         read_size);
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
        else
        {
            status = vm_mcal_serial_write(target, data, size);
        }
    }

    return status;
}
