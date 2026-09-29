/*
 * 文件说明：自定义变量监控协议服务实现，完成协议拆包、组包和 PduR 收发适配。
 * 所属模块：通信模块 / Services 层 / Custom 协议。
 * 设计要点：协议服务只与 PduR 交互；COM 请求由 PduR 路由进来，协议响应通过 PduR 上报 COM。
 */

#include "vm_custom_service.h"

#include "vm_custom_protocol.h"

#include <string.h>

/* 常量说明：自定义协议单帧最大长度，等于 1024 字节负载加 13 字节帧头/CRC。 */
#define VM_CUSTOM_SERVICE_FRAME_MAX (1037u)

/* 类型说明：自定义协议服务对象内部结构。 */
struct vm_custom_service
{
    /* 变量说明：所在 PduR 控制块。 */
    vm_pdur_t *pdur;
    /* 变量说明：注册到 PduR 的服务节点。 */
    vm_pdur_service_t node;
    /* 变量说明：流式协议解析器，用于处理串口粘包和半包。 */
    vm_custom_parser_t parser;
    /* 变量说明：下层设备类型。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：通信通道号，用于让 PduR 匹配具体下层路由。 */
    uint32_t channel;
    /* 变量说明：静态服务节点是否已注册到 PduR。 */
    uint8_t registered;
};

static uint8_t vm_custom_service_match(
    void *context,
    const vm_pdur_context_t *pdur_context);
static vm_status_t vm_custom_service_process(
    void *context,
    vm_pdur_context_t *pdur_context);
static vm_status_t vm_custom_service_request(
    void *context,
    const vm_pdur_message_t *message);
static vm_status_t vm_custom_service_on_message(
    void *context,
    const vm_custom_message_t *message);

/* 变量说明：自定义协议服务静态节点，删除 Services 层注册表中的 custom 项即可禁用本协议。 */
static vm_custom_service_t g_vm_custom_service;

vm_status_t vm_custom_service_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    uint32_t channel,
    vm_custom_service_t **out_service)
{
    vm_custom_service_t *service;
    vm_status_t status;

    if (out_service == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out_service = NULL;
        if (pdur == NULL)
        {
            status = VM_INVALID;
        }
        else
        {
            service = &g_vm_custom_service;
            if (service->registered != 0u)
            {
                status = VM_BUSY;
            }
            else
            {
                (void)memset(service, 0, sizeof(*service));
                service->pdur = pdur;
                service->device_type = device_type;
                service->channel = channel;
                service->node.service_id = VM_PDUR_SERVICE_CUSTOM;
                service->node.device_type = device_type;
                service->node.channel = channel;
                service->node.match = vm_custom_service_match;
                service->node.process = vm_custom_service_process;
                service->node.request = vm_custom_service_request;
                service->node.context = service;

                status = vm_custom_parser_init(&service->parser);
                if (status == VM_OK)
                {
                    status = vm_pdur_register_service(pdur, &service->node);
                }

                if (status == VM_OK)
                {
                    service->registered = 1u;
                    *out_service = service;
                }
                else
                {
                    (void)memset(service, 0, sizeof(*service));
                }
            }
        }
    }

    return status;
}

void vm_custom_service_destroy(vm_custom_service_t *service)
{
    if (service != NULL)
    {
        if (service->pdur != NULL)
        {
            (void)vm_pdur_unregister_service(service->pdur, &service->node);
        }
        vm_custom_parser_reset(&service->parser);
        (void)memset(service, 0, sizeof(*service));
    }
}

static uint8_t vm_custom_service_match(
    void *context,
    const vm_pdur_context_t *pdur_context)
{
    const vm_custom_service_t *service;
    uint8_t matched;

    service = (const vm_custom_service_t *)context;
    matched = 0u;
    if ((service != NULL) && (pdur_context != NULL))
    {
        if ((pdur_context->device_type == service->device_type) &&
            (pdur_context->channel == service->channel) &&
            ((pdur_context->service_id == VM_PDUR_SERVICE_ANY) ||
             (pdur_context->service_id == VM_PDUR_SERVICE_CUSTOM)))
        {
            matched = 1u;
        }
    }

    return matched;
}

static vm_status_t vm_custom_service_process(
    void *context,
    vm_pdur_context_t *pdur_context)
{
    vm_custom_service_t *service;
    vm_status_t status;

    service = (vm_custom_service_t *)context;
    if ((service == NULL) || (pdur_context == NULL) ||
        ((pdur_context->rx_data == NULL) && (pdur_context->rx_size > 0u)))
    {
        status = VM_INVALID;
    }
    else
    {
        status = vm_custom_parser_feed(&service->parser,
                                       pdur_context->rx_data,
                                       pdur_context->rx_size,
                                       vm_custom_service_on_message,
                                       service);
    }

    return status;
}

static vm_status_t vm_custom_service_request(
    void *context,
    const vm_pdur_message_t *message)
{
    vm_custom_service_t *service;
    vm_pdur_context_t pdur_context;
    uint8_t frame[VM_CUSTOM_SERVICE_FRAME_MAX];
    size_t written;
    vm_status_t status;

    service = (vm_custom_service_t *)context;
    if ((service == NULL) || (message == NULL))
    {
        status = VM_INVALID;
    }
    else if ((message->device_type != service->device_type) ||
             (message->channel != service->channel) ||
             (message->service_id != VM_PDUR_SERVICE_CUSTOM))
    {
        status = VM_NOT_FOUND;
    }
    else if (message->command == VM_PDUR_COMMAND_READ)
    {
        status = vm_custom_make_read(message->sequence,
                                     message->address,
                                     message->length,
                                     frame,
                                     sizeof(frame),
                                     &written);
    }
    else if (message->command == VM_PDUR_COMMAND_WRITE)
    {
        status = vm_custom_make_write(message->sequence,
                                      message->address,
                                      message->payload,
                                      message->length,
                                      frame,
                                      sizeof(frame),
                                      &written);
    }
    else
    {
        status = VM_UNSUPPORTED;
    }

    if (status == VM_OK)
    {
        pdur_context.device_type = service->device_type;
        pdur_context.service_id = VM_PDUR_SERVICE_CUSTOM;
        pdur_context.rx_data = NULL;
        pdur_context.rx_size = 0u;
        pdur_context.tx_data = frame;
        pdur_context.tx_size = written;
        pdur_context.channel = service->channel;
        status = vm_pdur_output(service->pdur, &pdur_context);
    }

    return status;
}

static vm_status_t vm_custom_service_on_message(
    void *context,
    const vm_custom_message_t *message)
{
    vm_custom_service_t *service;
    vm_pdur_message_t pdur_message;
    vm_status_t status;

    service = (vm_custom_service_t *)context;
    if ((service == NULL) || (message == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        pdur_message.device_type = service->device_type;
        pdur_message.service_id = VM_PDUR_SERVICE_CUSTOM;
        pdur_message.channel = service->channel;
        pdur_message.sequence = message->sequence;
        pdur_message.address = message->address;
        pdur_message.payload = message->payload;
        pdur_message.length = message->length;

        switch (message->command)
        {
            case VM_CUSTOM_READ_RESPONSE:
                pdur_message.command = VM_PDUR_COMMAND_READ_RESPONSE;
                break;

            case VM_CUSTOM_WRITE_RESPONSE:
                pdur_message.command = VM_PDUR_COMMAND_WRITE_RESPONSE;
                break;

            case VM_CUSTOM_ERROR:
                pdur_message.command = VM_PDUR_COMMAND_ERROR;
                break;

            default:
                pdur_message.command = message->command;
                break;
        }

        status = vm_pdur_com_indicate(service->pdur, &pdur_message);
    }

    return status;
}
