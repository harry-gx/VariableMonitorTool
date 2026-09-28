/*
 * 文件说明：Services 总入口实现，统一管理挂接到服务层的协议服务。
 * 所属模块：通信模块 / Services 层。
 * 设计要点：当前先挂接自定义协议，后续 XCP、UDS 继续在本层扩展，不影响 COM 层接口。
 */

#include "vm_service.h"

#include "vm_custom_service.h"

#include <stdlib.h>

/* 类型说明：Services 管理器内部结构。 */
struct vm_service_manager
{
    /* 变量说明：路由层控制块，所有服务收发都通过该 PduR 对象进入下层。 */
    vm_pdur_t *pdur;
    /* 变量说明：自定义变量监控协议服务实例。 */
    vm_custom_service_t *custom_service;
};

vm_status_t vm_service_manager_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    void *lower_layer,
    void *lower_device,
    vm_service_custom_message_fn custom_callback,
    void *custom_context,
    vm_service_manager_t **out_manager)
{
    vm_service_manager_t *manager;
    vm_status_t status;

    if (out_manager == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out_manager = NULL;
        if ((pdur == NULL) || (lower_layer == NULL) ||
            (lower_device == NULL) || (custom_callback == NULL))
        {
            status = VM_INVALID;
        }
        else
        {
            manager = (vm_service_manager_t *)calloc(1u, sizeof(*manager));
            if (manager == NULL)
            {
                status = VM_NOMEM;
            }
            else
            {
                manager->pdur = pdur;
                status = vm_custom_service_create(pdur,
                                                  device_type,
                                                  lower_layer,
                                                  lower_device,
                                                  custom_callback,
                                                  custom_context,
                                                  &manager->custom_service);
                if (status == VM_OK)
                {
                    *out_manager = manager;
                }
                else
                {
                    free(manager);
                }
            }
        }
    }

    return status;
}

void vm_service_manager_destroy(vm_service_manager_t *manager)
{
    if (manager != NULL)
    {
        vm_custom_service_destroy(manager->custom_service);
        manager->custom_service = NULL;
        manager->pdur = NULL;
        free(manager);
    }
}

vm_status_t vm_service_custom_read(vm_service_manager_t *manager,
                                   uint8_t sequence,
                                   uint32_t address,
                                   uint16_t length)
{
    vm_status_t status;

    if ((manager == NULL) || (manager->custom_service == NULL))
    {
        status = VM_BUSY;
    }
    else
    {
        status = vm_custom_service_send_read(manager->custom_service,
                                            sequence,
                                            address,
                                            length);
    }

    return status;
}

vm_status_t vm_service_custom_write(vm_service_manager_t *manager,
                                    uint8_t sequence,
                                    uint32_t address,
                                    const uint8_t *data,
                                    uint16_t length)
{
    vm_status_t status;

    if ((manager == NULL) || (manager->custom_service == NULL))
    {
        status = VM_BUSY;
    }
    else
    {
        status = vm_custom_service_send_write(manager->custom_service,
                                             sequence,
                                             address,
                                             data,
                                             length);
    }

    return status;
}
