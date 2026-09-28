/*
 * 文件说明：自定义变量监控协议服务实现，完成协议拆包、组包和 PduR 收发适配。
 * 所属模块：通信模块 / Services 层。
 * 设计要点：服务层内部持有协议解析器，COM 层只关心读写请求和解析后的消息事件。
 */

#include "vm_custom_service.h"

#include <stdlib.h>

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
    vm_custom_parser_t *parser;
    /* 变量说明：下层设备类型。 */
    vm_pdur_device_type_t device_type;
    /* 变量说明：下层抽象层对象，例如串口 IF。 */
    void *lower_layer;
    /* 变量说明：下层具体设备对象，例如当前串口设备。 */
    void *lower_device;
    /* 变量说明：解析出完整协议消息后的上层回调。 */
    vm_custom_service_message_fn callback;
    /* 变量说明：上层回调上下文。 */
    void *callback_context;
};

static uint8_t vm_custom_service_match(
    void *context,
    const vm_pdur_context_t *pdur_context);
static vm_status_t vm_custom_service_process(
    void *context,
    vm_pdur_context_t *pdur_context);
static vm_status_t vm_custom_service_on_message(
    void *context,
    const vm_custom_message_t *message);
static vm_status_t vm_custom_service_send_frame(
    vm_custom_service_t *service,
    const uint8_t *frame,
    size_t frame_size);

vm_status_t vm_custom_service_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    void *lower_layer,
    void *lower_device,
    vm_custom_service_message_fn callback,
    void *callback_context,
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
        if ((pdur == NULL) || (lower_layer == NULL) || (lower_device == NULL) ||
            (callback == NULL))
        {
            status = VM_INVALID;
        }
        else
        {
            service = (vm_custom_service_t *)calloc(1u, sizeof(*service));
            if (service == NULL)
            {
                status = VM_NOMEM;
            }
            else
            {
                service->pdur = pdur;
                service->device_type = device_type;
                service->lower_layer = lower_layer;
                service->lower_device = lower_device;
                service->callback = callback;
                service->callback_context = callback_context;
                service->node.service_id = VM_PDUR_SERVICE_CUSTOM;
                service->node.device_type = device_type;
                service->node.match = vm_custom_service_match;
                service->node.process = vm_custom_service_process;
                service->node.context = service;

                status = vm_custom_parser_create(&service->parser);
                if (status == VM_OK)
                {
                    status = vm_pdur_register_service(pdur, &service->node);
                }

                if (status == VM_OK)
                {
                    *out_service = service;
                }
                else
                {
                    vm_custom_parser_destroy(service->parser);
                    free(service);
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
        vm_custom_parser_destroy(service->parser);
        free(service);
    }
}

vm_status_t vm_custom_service_send_read(vm_custom_service_t *service,
                                        uint8_t sequence,
                                        uint32_t address,
                                        uint16_t length)
{
    uint8_t frame[VM_CUSTOM_SERVICE_FRAME_MAX];
    size_t written;
    vm_status_t status;

    if (service == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        status = vm_custom_make_read(sequence,
                                     address,
                                     length,
                                     frame,
                                     sizeof(frame),
                                     &written);
        if (status == VM_OK)
        {
            status = vm_custom_service_send_frame(service, frame, written);
        }
    }

    return status;
}

vm_status_t vm_custom_service_send_write(vm_custom_service_t *service,
                                         uint8_t sequence,
                                         uint32_t address,
                                         const uint8_t *data,
                                         uint16_t length)
{
    uint8_t frame[VM_CUSTOM_SERVICE_FRAME_MAX];
    size_t written;
    vm_status_t status;

    if (service == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        status = vm_custom_make_write(sequence,
                                      address,
                                      data,
                                      length,
                                      frame,
                                      sizeof(frame),
                                      &written);
        if (status == VM_OK)
        {
            status = vm_custom_service_send_frame(service, frame, written);
        }
    }

    return status;
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
            ((pdur_context->service_id == 0u) ||
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
    if ((service == NULL) || (service->parser == NULL) ||
        (pdur_context == NULL) ||
        ((pdur_context->rx_data == NULL) && (pdur_context->rx_size > 0u)))
    {
        status = VM_INVALID;
    }
    else
    {
        status = vm_custom_parser_feed(service->parser,
                                       pdur_context->rx_data,
                                       pdur_context->rx_size,
                                       vm_custom_service_on_message,
                                       service);
    }

    return status;
}

static vm_status_t vm_custom_service_on_message(
    void *context,
    const vm_custom_message_t *message)
{
    vm_custom_service_t *service;
    vm_status_t status;

    service = (vm_custom_service_t *)context;
    if ((service == NULL) || (service->callback == NULL) ||
        (message == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        status = service->callback(service->callback_context, message);
    }

    return status;
}

static vm_status_t vm_custom_service_send_frame(
    vm_custom_service_t *service,
    const uint8_t *frame,
    size_t frame_size)
{
    vm_pdur_context_t pdur_context;
    vm_status_t status;

    if ((service == NULL) || ((frame == NULL) && (frame_size > 0u)))
    {
        status = VM_INVALID;
    }
    else
    {
        pdur_context.device_type = service->device_type;
        pdur_context.service_id = VM_PDUR_SERVICE_CUSTOM;
        pdur_context.lower_layer = service->lower_layer;
        pdur_context.lower_device = service->lower_device;
        pdur_context.rx_data = NULL;
        pdur_context.rx_size = 0u;
        pdur_context.tx_data = frame;
        pdur_context.tx_size = frame_size;
        pdur_context.channel = 0u;
        status = vm_pdur_output(service->pdur, &pdur_context);
    }

    return status;
}
