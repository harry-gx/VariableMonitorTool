/*
 * 文件说明：PduR 路由层实现，负责把 IF/TP 收到的数据分发给服务，并把服务输出送回下层。
 * 所属模块：通信模块 / PduR 层。
 * 设计要点：服务表使用通信模块内拷贝的 vm_list.h 侵入式链表，服务节点由具体服务对象内嵌和管理。
 */

#include "vm_pdur.h"

#include "vm_serial_if.h"

#include <stdlib.h>

struct vm_pdur
{
    list_head_t service_list;
};

static uint8_t vm_pdur_service_conflict(const vm_pdur_service_t *left,
                                        const vm_pdur_service_t *right)
{
    uint8_t conflict;

    conflict = 0u;
    if ((left != NULL) && (right != NULL))
    {
        if ((left == right) ||
            ((left->device_type == right->device_type) &&
             (left->service_id == right->service_id)))
        {
            conflict = 1u;
        }
    }

    return conflict;
}

vm_status_t vm_pdur_create(vm_pdur_t **out_pdur)
{
    vm_pdur_t *pdur;
    vm_status_t status;

    if (out_pdur == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out_pdur = NULL;
        pdur = (vm_pdur_t *)calloc(1u, sizeof(*pdur));
        if (pdur == NULL)
        {
            status = VM_NOMEM;
        }
        else
        {
            INIT_LIST_HEAD(&pdur->service_list);
            *out_pdur = pdur;
            status = VM_OK;
        }
    }

    return status;
}

void vm_pdur_destroy(vm_pdur_t *pdur)
{
    if (pdur != NULL)
    {
        free(pdur);
    }
}

vm_status_t vm_pdur_register_service(vm_pdur_t *pdur,
                                     vm_pdur_service_t *service)
{
    list_head_t *node;
    vm_pdur_service_t *current;
    vm_status_t status;

    if ((pdur == NULL) || (service == NULL) || (service->process == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = VM_OK;
        node = pdur->service_list.next;
        while (node != &pdur->service_list)
        {
            current = list_entry(node, vm_pdur_service_t, node);
            if (vm_pdur_service_conflict(current, service) != 0u)
            {
                status = VM_DUPLICATE;
                break;
            }
            node = node->next;
        }

        if (status == VM_OK)
        {
            INIT_LIST_HEAD(&service->node);
            LIST_ADD_TAIL(&service->node, &pdur->service_list);
        }
    }

    return status;
}

vm_status_t vm_pdur_unregister_service(vm_pdur_t *pdur,
                                       vm_pdur_service_t *service)
{
    list_head_t *node;
    vm_pdur_service_t *current;
    vm_status_t status;

    if ((pdur == NULL) || (service == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = VM_NOT_FOUND;
        node = pdur->service_list.next;
        while (node != &pdur->service_list)
        {
            current = list_entry(node, vm_pdur_service_t, node);
            if (current == service)
            {
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

vm_status_t vm_pdur_input(vm_pdur_t *pdur, vm_pdur_context_t *context)
{
    list_head_t *node;
    vm_pdur_service_t *service;
    vm_status_t status;
    vm_status_t service_status;
    uint8_t matched;

    if ((pdur == NULL) || (context == NULL) ||
        ((context->rx_data == NULL) && (context->rx_size > 0u)))
    {
        status = VM_INVALID;
    }
    else
    {
        status = VM_NOT_FOUND;
        node = pdur->service_list.next;
        while (node != &pdur->service_list)
        {
            service = list_entry(node, vm_pdur_service_t, node);
            matched = 0u;
            if (service->device_type == context->device_type)
            {
                if (service->match == NULL)
                {
                    matched = 1u;
                }
                else
                {
                    matched = service->match(service->context, context);
                }
            }

            if (matched != 0u)
            {
                service_status = service->process(service->context, context);
                if (service_status == VM_OK)
                {
                    status = VM_OK;
                }
                else if (status == VM_NOT_FOUND)
                {
                    status = service_status;
                }
                else
                {
                    /* 已有服务成功处理，本服务失败不覆盖整体成功状态。 */
                }
            }
            node = node->next;
        }
    }

    return status;
}

vm_status_t vm_pdur_output(vm_pdur_t *pdur,
                           const vm_pdur_context_t *context)
{
    vm_status_t status;

    (void)pdur;
    if ((context == NULL) || ((context->tx_data == NULL) &&
                              (context->tx_size > 0u)))
    {
        status = VM_INVALID;
    }
    else
    {
        switch (context->device_type)
        {
            case VM_PDUR_DEVICE_SERIAL:
                status = vm_serial_if_write(
                    (vm_serial_if_t *)context->lower_layer,
                    (vm_mcal_serial_device_t *)context->lower_device,
                    context->tx_data,
                    context->tx_size);
                break;

            case VM_PDUR_DEVICE_CAN:
            case VM_PDUR_DEVICE_CANFD:
            case VM_PDUR_DEVICE_ETHERNET:
            default:
                status = VM_UNSUPPORTED;
                break;
        }
    }

    return status;
}
