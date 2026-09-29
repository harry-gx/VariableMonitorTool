/*
 * 文件说明：串口 IF 层实现，统一管理当前平台串口设备模板和运行时 MCAL 设备对象。
 * 所属模块：通信模块 / IF 层。
 * 设计要点：IF 层只维护静态运行时设备节点，当前平台模板用指针保存，不在本层申请堆内存。
 */

#include "vm_serial_if.h"

#include "vm_list.h"
#include "vm_mcal_serial_platform.h"

#include <stddef.h>
#include <string.h>

/* 常量说明：平台私有上下文静态存储容量，覆盖 Windows HANDLE 和 POSIX fd 上下文。 */
#define VM_SERIAL_IF_CONTEXT_STORAGE_SIZE (128u)

/* 类型说明：串口 IF 控制块。 */
struct vm_serial_if
{
    const vm_mcal_serial_device_t *platform_device;
    list_head_t device_list;
    vm_mcal_serial_device_t *active_device;
    uint8_t initialized;
};

/* 类型说明：用于保证平台上下文静态存储具备足够对齐。 */
typedef union
{
    max_align_t align;
    uint8_t bytes[VM_SERIAL_IF_CONTEXT_STORAGE_SIZE];
} vm_serial_if_context_storage_t;

/* 变量说明：Serial IF 静态控制块。 */
static vm_serial_if_t g_vm_serial_if_instance;
/* 变量说明：Serial IF 静态运行时设备节点。 */
static vm_mcal_serial_device_t g_vm_serial_if_runtime_device;
/* 变量说明：平台私有上下文静态存储。 */
static vm_serial_if_context_storage_t g_vm_serial_if_context_storage;
/* 变量说明：运行时设备槽是否已被注册。 */
static uint8_t g_vm_serial_if_device_registered;

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
    if ((serial_if != NULL) && (device != NULL) &&
        (serial_if->device_list.next != NULL))
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
    if ((serial_if != NULL) && (device_name != NULL) &&
        (serial_if->device_list.next != NULL))
    {
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            current = list_entry(node, vm_mcal_serial_device_t, node);
            if (strcmp(current->config.device_name, device_name) == 0)
            {
                exists = 1u;
                break;
            }
            node = node->next;
        }
    }

    return exists;
}

static void vm_serial_if_device_reset(vm_mcal_serial_device_t *device)
{
    if (device != NULL)
    {
        if ((device->ops != NULL) && (device->ops->close != NULL))
        {
            (void)device->ops->close(device);
        }
        (void)memset(device, 0, sizeof(*device));
        (void)memset(&g_vm_serial_if_context_storage,
                     0,
                     sizeof(g_vm_serial_if_context_storage));
        g_vm_serial_if_device_registered = 0u;
    }
}

static void vm_serial_if_clear_devices(vm_serial_if_t *serial_if)
{
    list_head_t *node;
    list_head_t *next_node;
    vm_mcal_serial_device_t *device;

    if ((serial_if != NULL) && (serial_if->device_list.next != NULL))
    {
        node = serial_if->device_list.next;
        while (node != &serial_if->device_list)
        {
            next_node = node->next;
            device = list_entry(node, vm_mcal_serial_device_t, node);
            LIST_DEL(&device->node);
            INIT_LIST_HEAD(&device->node);
            vm_serial_if_device_reset(device);
            node = next_node;
        }
        serial_if->active_device = NULL;
        INIT_LIST_HEAD(&serial_if->device_list);
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
    else if ((platform_device->platform_context_size == 0u) ||
             (platform_device->platform_context_size >
              (size_t)VM_SERIAL_IF_CONTEXT_STORAGE_SIZE) ||
             (vm_serial_if_device_ops_valid(platform_device) == 0u))
    {
        status = VM_UNSUPPORTED;
    }
    else
    {
        (void)memset(device, 0, sizeof(*device));
        (void)memset(&g_vm_serial_if_context_storage,
                     0,
                     sizeof(g_vm_serial_if_context_storage));
        INIT_LIST_HEAD(&device->node);
        device->platform_name = platform_device->platform_name;
        device->ops = platform_device->ops;
        device->platform_context_size = platform_device->platform_context_size;
        device->platform_context = g_vm_serial_if_context_storage.bytes;
        device->config = *config;
        device->config.device_name[sizeof(device->config.device_name) - 1u] = '\0';
        status = VM_OK;
    }

    return status;
}

vm_status_t vm_serial_if_create(vm_serial_if_t **out_if)
{
    const vm_mcal_serial_device_t *platform_device;
    vm_status_t status;

    if (out_if == NULL)
    {
        status = VM_INVALID;
    }
    else if (g_vm_serial_if_instance.initialized != 0u)
    {
        *out_if = NULL;
        status = VM_BUSY;
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
            (void)memset(&g_vm_serial_if_instance,
                         0,
                         sizeof(g_vm_serial_if_instance));
            (void)memset(&g_vm_serial_if_runtime_device,
                         0,
                         sizeof(g_vm_serial_if_runtime_device));
            (void)memset(&g_vm_serial_if_context_storage,
                         0,
                         sizeof(g_vm_serial_if_context_storage));
            g_vm_serial_if_device_registered = 0u;
            g_vm_serial_if_instance.platform_device = platform_device;
            INIT_LIST_HEAD(&g_vm_serial_if_instance.device_list);
            g_vm_serial_if_instance.initialized = 1u;
            *out_if = &g_vm_serial_if_instance;
            status = VM_OK;
        }
    }

    return status;
}

void vm_serial_if_destroy(vm_serial_if_t *serial_if)
{
    if (serial_if == &g_vm_serial_if_instance)
    {
        vm_serial_if_close(serial_if, NULL);
        vm_serial_if_clear_devices(serial_if);
        (void)memset(serial_if, 0, sizeof(*serial_if));
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
        else if (g_vm_serial_if_device_registered != 0u)
        {
            status = VM_BUSY;
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
            device = &g_vm_serial_if_runtime_device;
            status = vm_serial_if_apply_config(serial_if->platform_device == NULL ?
                                               NULL : device,
                                               serial_if->platform_device,
                                               config);
            if (status == VM_OK)
            {
                LIST_ADD_TAIL(&device->node, &serial_if->device_list);
                g_vm_serial_if_device_registered = 1u;
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
        vm_serial_if_device_reset(device);
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
        if (vm_serial_if_device_ops_valid(device) == 0u)
        {
            status = VM_INVALID;
        }
        else
        {
            status = device->ops->open(device);
        }
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
            if ((target->ops != NULL) && (target->ops->close != NULL))
            {
                (void)target->ops->close(target);
            }
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
