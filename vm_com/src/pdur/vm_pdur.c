/*
 * 文件说明：PduR 路由层实现，负责 COM、服务协议和下层通道之间的数据路由。
 * 所属模块：通信模块 / PduR 层。
 * 设计要点：COM 不直接调用协议，协议也不直接回调 COM，二者只通过 PduR 的逻辑消息接口交换数据。
 *           下层路由节点和协议服务节点均由各模块静态定义，PduR 只负责显式挂接和路由。
 */

#include "vm_pdur.h"

#include <string.h>

/* 常量说明：PduR 从下层单次读取的最大字节数，覆盖当前自定义协议最大帧。 */
#define VM_PDUR_RX_BUFFER_SIZE (2048u)

/* 类型说明：PduR 控制块内部结构。 */
struct vm_pdur
{
    /* 变量说明：协议服务链表，CUSTOM/XCP/UDS 等服务挂接在这里。 */
    list_head_t service_list;
    /* 变量说明：下层路由链表，SERIAL_IF/CAN_TP/ETH_IF 等通道挂接在这里。 */
    list_head_t lower_route_list;
    /* 变量说明：COM 上报回调配置。 */
    vm_pdur_com_config_t com_config;
    /* 变量说明：COM 是否已经注册，1 表示可向 COM 上报逻辑消息。 */
    uint8_t com_registered;
    /* 变量说明：控制块是否已初始化。 */
    uint8_t initialized;
    /* 变量说明：轮询下层时使用的临时接收缓冲区。 */
    uint8_t rx_buffer[VM_PDUR_RX_BUFFER_SIZE];
};

/* 类型说明：PduR 显式设备路由表项。 */
typedef struct
{
    /* 变量说明：设备类型，顺序与 vm_pdur_device_type_t 保持一致便于阅读。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：是否启用该设备类型输出路由，0 表示当前不支持。 */
    uint8_t enabled;
} vm_pdur_device_info_t;

static void vm_pdur_reset(vm_pdur_t *pdur);

/*
 * 变量说明：PduR 显式设备路由表。
 * 修改说明：删除某个设备类型表项或把 enabled 置 0 后，该类型的输出路由不可用。
 */
static const vm_pdur_device_info_t g_vm_pdur_device_info[] =
{
    { VM_PDUR_DEVICE_SERIAL, 1u },
    { VM_PDUR_DEVICE_CAN, 0u },
    { VM_PDUR_DEVICE_CANFD, 0u },
    { VM_PDUR_DEVICE_ETHERNET, 0u }
};

/* 变量说明：PduR 静态控制块，通信模块当前只需要一个协议路由实例。 */
static vm_pdur_t g_vm_pdur_instance;

static void vm_pdur_reset(vm_pdur_t *pdur)
{
    list_head_t *node;
    list_head_t *next;
    vm_pdur_lower_route_t *route;
    vm_pdur_service_t *service;

    if (pdur != NULL)
    {
        if (pdur->lower_route_list.next != NULL)
        {
            node = pdur->lower_route_list.next;
            while (node != &pdur->lower_route_list)
            {
                next = node->next;
                route = list_entry(node, vm_pdur_lower_route_t, node);
                LIST_DEL(&route->node);
                INIT_LIST_HEAD(&route->node);
                route->registered = 0u;
                node = next;
            }
        }

        if (pdur->service_list.next != NULL)
        {
            node = pdur->service_list.next;
            while (node != &pdur->service_list)
            {
                next = node->next;
                service = list_entry(node, vm_pdur_service_t, node);
                LIST_DEL(&service->node);
                INIT_LIST_HEAD(&service->node);
                service->registered = 0u;
                node = next;
            }
        }

        (void)memset(pdur, 0, sizeof(*pdur));
    }
}

vm_status_t vm_pdur_create(vm_pdur_t **out_pdur)
{
    vm_status_t status;

    if (out_pdur == NULL)
    {
        status = VM_INVALID;
    }
    else if (g_vm_pdur_instance.initialized != 0u)
    {
        *out_pdur = NULL;
        status = VM_BUSY;
    }
    else
    {
        (void)memset(&g_vm_pdur_instance, 0, sizeof(g_vm_pdur_instance));
        INIT_LIST_HEAD(&g_vm_pdur_instance.service_list);
        INIT_LIST_HEAD(&g_vm_pdur_instance.lower_route_list);
        g_vm_pdur_instance.initialized = 1u;
        *out_pdur = &g_vm_pdur_instance;
        status = VM_OK;
    }

    return status;
}

void vm_pdur_destroy(vm_pdur_t *pdur)
{
    if (pdur == &g_vm_pdur_instance)
    {
        vm_pdur_reset(pdur);
    }
}

vm_status_t vm_pdur_register_lower_route(vm_pdur_t *pdur,
                                         vm_pdur_lower_route_t *route)
{
    list_head_t *node;
    vm_pdur_lower_route_t *current;
    vm_status_t status;

    if ((pdur == NULL) || (route == NULL) ||
        (route->config.read == NULL) || (route->config.write == NULL))
    {
        status = VM_INVALID;
    }
    else if (route->registered != 0u)
    {
        status = VM_DUPLICATE;
    }
    else
    {
        status = VM_OK;
        node = pdur->lower_route_list.next;
        while (node != &pdur->lower_route_list)
        {
            current = list_entry(node, vm_pdur_lower_route_t, node);
            if ((current == route) ||
                ((current->config.device_type == route->config.device_type) &&
                 (current->config.channel == route->config.channel)))
            {
                status = VM_DUPLICATE;
                break;
            }
            node = node->next;
        }

        if (status == VM_OK)
        {
            INIT_LIST_HEAD(&route->node);
            LIST_ADD_TAIL(&route->node, &pdur->lower_route_list);
            route->registered = 1u;
        }
    }

    return status;
}

vm_status_t vm_pdur_unregister_lower_route(vm_pdur_t *pdur,
                                           vm_pdur_lower_route_t *route)
{
    list_head_t *node;
    vm_pdur_lower_route_t *current;
    vm_status_t status;

    if ((pdur == NULL) || (route == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = VM_NOT_FOUND;
        node = pdur->lower_route_list.next;
        while (node != &pdur->lower_route_list)
        {
            current = list_entry(node, vm_pdur_lower_route_t, node);
            if (current == route)
            {
                LIST_DEL(&current->node);
                INIT_LIST_HEAD(&current->node);
                current->registered = 0u;
                status = VM_OK;
                break;
            }
            node = node->next;
        }
    }

    return status;
}

vm_status_t vm_pdur_register_com_route(vm_pdur_t *pdur,
                                       const vm_pdur_com_config_t *config)
{
    vm_status_t status;

    if ((pdur == NULL) || (config == NULL) || (config->indication == NULL))
    {
        status = VM_INVALID;
    }
    else if (pdur->com_registered != 0u)
    {
        status = VM_DUPLICATE;
    }
    else
    {
        pdur->com_config = *config;
        pdur->com_registered = 1u;
        status = VM_OK;
    }

    return status;
}

void vm_pdur_unregister_com_route(vm_pdur_t *pdur)
{
    if (pdur != NULL)
    {
        pdur->com_registered = 0u;
        pdur->com_config.indication = NULL;
        pdur->com_config.context = NULL;
    }
}

vm_status_t vm_pdur_register_service(vm_pdur_t *pdur,
                                     vm_pdur_service_t *service)
{
    list_head_t *node;
    vm_pdur_service_t *current;
    vm_status_t status;

    if ((pdur == NULL) || (service == NULL) ||
        ((service->process == NULL) && (service->request == NULL)))
    {
        status = VM_INVALID;
    }
    else if (service->registered != 0u)
    {
        status = VM_DUPLICATE;
    }
    else
    {
        status = VM_OK;
        node = pdur->service_list.next;
        while (node != &pdur->service_list)
        {
            current = list_entry(node, vm_pdur_service_t, node);
            if ((current == service) ||
                ((current->device_type == service->device_type) &&
                 (current->channel == service->channel) &&
                 (current->service_id == service->service_id)))
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
            service->registered = 1u;
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
                current->registered = 0u;
                status = VM_OK;
                break;
            }
            node = node->next;
        }
    }

    return status;
}

vm_status_t vm_pdur_poll(vm_pdur_t *pdur, uint8_t budget)
{
    list_head_t *node;
    vm_pdur_lower_route_t *route;
    vm_pdur_context_t context;
    size_t read_size;
    vm_status_t status;
    uint8_t processed;
    uint8_t any_processed;
    uint8_t remaining;

    if (pdur == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        status = VM_OK;
        remaining = budget;
        while (remaining > 0u)
        {
            any_processed = 0u;
            node = pdur->lower_route_list.next;
            while ((node != &pdur->lower_route_list) && (remaining > 0u))
            {
                route = list_entry(node, vm_pdur_lower_route_t, node);
                node = node->next;
                processed = 0u;
                read_size = 0u;
                if (route->config.read == NULL)
                {
                    status = VM_INVALID;
                }
                else
                {
                    status = route->config.read(route->config.lower_layer,
                                                route->config.lower_device,
                                                pdur->rx_buffer,
                                                sizeof(pdur->rx_buffer),
                                                &read_size);
                }

                if (status == VM_NOT_FOUND)
                {
                    status = VM_OK;
                }
                else if ((status == VM_OK) && (read_size > 0u))
                {
                    context.device_type = route->config.device_type;
                    context.service_id = VM_PDUR_SERVICE_ANY;
                    context.rx_data = pdur->rx_buffer;
                    context.rx_size = read_size;
                    context.tx_data = NULL;
                    context.tx_size = 0u;
                    context.channel = route->config.channel;
                    status = vm_pdur_input(pdur, &context);
                    if (status == VM_NOT_FOUND)
                    {
                        status = VM_OK;
                    }
                    if (status == VM_OK)
                    {
                        processed = 1u;
                    }
                }
                else
                {
                    /* 无数据或读取失败时保留 status，由下方统一处理。 */
                }

                if (status != VM_OK)
                {
                    remaining = 0u;
                    break;
                }
                if (processed != 0u)
                {
                    any_processed = 1u;
                    --remaining;
                }
            }
            if (any_processed == 0u)
            {
                break;
            }
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
            if ((service->device_type == context->device_type) &&
                (service->channel == context->channel))
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
    list_head_t *node;
    vm_pdur_lower_route_t *route;
    size_t index;
    uint8_t enabled;
    vm_status_t status;

    if ((pdur == NULL) || (context == NULL) ||
        ((context->tx_data == NULL) && (context->tx_size > 0u)))
    {
        status = VM_INVALID;
    }
    else
    {
        enabled = 0u;
        for (index = 0u;
             index < (sizeof(g_vm_pdur_device_info) / sizeof(g_vm_pdur_device_info[0]));
             ++index)
        {
            if (g_vm_pdur_device_info[index].device_type == context->device_type)
            {
                enabled = g_vm_pdur_device_info[index].enabled;
                break;
            }
        }

        if (enabled == 0u)
        {
            status = VM_UNSUPPORTED;
        }
        else
        {
            status = VM_NOT_FOUND;
            node = pdur->lower_route_list.next;
            while (node != &pdur->lower_route_list)
            {
                route = list_entry(node, vm_pdur_lower_route_t, node);
                if ((route->config.device_type == context->device_type) &&
                    (route->config.channel == context->channel))
                {
                    if (route->config.write == NULL)
                    {
                        status = VM_UNSUPPORTED;
                    }
                    else
                    {
                        status = route->config.write(route->config.lower_layer,
                                                     route->config.lower_device,
                                                     context->tx_data,
                                                     context->tx_size);
                    }
                    break;
                }
                node = node->next;
            }
        }
    }

    return status;
}

vm_status_t vm_pdur_service_request(vm_pdur_t *pdur,
                                    const vm_pdur_message_t *message)
{
    list_head_t *node;
    vm_pdur_service_t *service;
    vm_status_t status;

    if ((pdur == NULL) || (message == NULL))
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
            if ((service->device_type == message->device_type) &&
                (service->channel == message->channel) &&
                (service->service_id == message->service_id))
            {
                if (service->request == NULL)
                {
                    status = VM_UNSUPPORTED;
                }
                else
                {
                    status = service->request(service->context, message);
                }
                break;
            }
            node = node->next;
        }
    }

    return status;
}

vm_status_t vm_pdur_com_indicate(vm_pdur_t *pdur,
                                 const vm_pdur_message_t *message)
{
    vm_status_t status;

    if ((pdur == NULL) || (message == NULL))
    {
        status = VM_INVALID;
    }
    else if ((pdur->com_registered == 0u) ||
             (pdur->com_config.indication == NULL))
    {
        status = VM_NOT_FOUND;
    }
    else
    {
        status = pdur->com_config.indication(pdur->com_config.context,
                                             message);
    }

    return status;
}
