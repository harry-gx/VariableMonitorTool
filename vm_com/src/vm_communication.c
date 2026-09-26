#include "vm_communication.h"

#include "vm_custom_protocol.h"
#include "vm_router.h"
#include "vm_serial_transport.h"
#include "vm_value_codec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VM_COMM_SERIAL_CONNECTION_ID (1u)
#define VM_COMM_CUSTOM_SERIAL_ROUTE_ID (1u)
#define VM_COMM_SERIAL_CHANNEL (0u)
#define VM_COMM_SERIAL_ADDRESS (0u)
#define VM_COMM_FRAME_MAX (1037u)
#define VM_COMM_PAYLOAD_MAX (1024u)
#define VM_COMM_PENDING_MAX (32u)
#define VM_COMM_DEFAULT_DATA_BITS (8u)
#define VM_COMM_DEFAULT_STOP_BITS (1u)
#define VM_COMM_DEFAULT_BAUDRATE (115200u)

typedef enum
{
    VM_COMM_PENDING_NONE = 0,
    VM_COMM_PENDING_READ,
    VM_COMM_PENDING_WRITE,
    VM_COMM_PENDING_BITFIELD_READ
} vm_comm_pending_kind_t;

typedef struct
{
    char name[VM_COMM_NAME_MAX];
    char type_name[VM_COMM_NAME_MAX];
    uint32_t address;
    uint16_t size;
    uint8_t writable;
    uint8_t monitorable;
    uint8_t calibratable;
    uint8_t bit_field;
    uint8_t bit_offset;
    uint8_t bit_size;
} vm_comm_variable_store_t;

typedef struct
{
    uint8_t used;
    uint8_t sequence;
    vm_comm_pending_kind_t kind;
    vm_comm_variable_store_t variable;
    char target_text[VM_COMM_TEXT_MAX];
} vm_comm_pending_t;

struct vm_comm
{
    vm_serial_transport_t *serial;
    vm_router_t *router;
    vm_custom_parser_t *parser;
    vm_comm_event_fn callback;
    void *callback_context;
    vm_comm_device_config_t config;
    char serial_device[VM_COMM_TEXT_MAX];
    char can_adapter[VM_COMM_TEXT_MAX];
    char network_host[VM_COMM_TEXT_MAX];
    uint8_t sequence;
    vm_comm_pending_t pending[VM_COMM_PENDING_MAX];
};

static vm_status_t vm_comm_on_message(void *context,
                                      const vm_custom_message_t *message);
static vm_status_t vm_comm_on_router_protocol_receive(void *context,
                                                      const vm_frame_t *frame);
static vm_status_t vm_comm_on_router_transport_send(void *context,
                                                    const vm_frame_t *frame);
static vm_status_t vm_comm_on_transport_packet(
    void *context,
    const vm_transport_packet_t *packet);
static vm_status_t vm_comm_connect_serial(vm_comm_t *comm);
static vm_status_t vm_comm_send_read(vm_comm_t *comm,
                                     const vm_comm_variable_store_t *variable,
                                     vm_comm_pending_kind_t kind,
                                     const char *target_text);
static vm_status_t vm_comm_send_write_bytes(vm_comm_t *comm,
                                            const vm_comm_variable_store_t *var,
                                            const uint8_t *data,
                                            uint16_t size);
static vm_status_t vm_comm_handle_read_response(vm_comm_t *comm,
                                                const vm_custom_message_t *msg,
                                                vm_comm_pending_t *pending);
static vm_status_t vm_comm_handle_write_response(vm_comm_t *comm,
                                                 const vm_custom_message_t *msg,
                                                 vm_comm_pending_t *pending);
static vm_status_t vm_comm_handle_error(vm_comm_t *comm,
                                        const vm_custom_message_t *msg,
                                        vm_comm_pending_t *pending);

static void vm_comm_copy_text(char *out, size_t out_size, const char *input)
{
    if ((out != NULL) && (out_size > 0u))
    {
        if (input == NULL)
        {
            out[0] = '\0';
        }
        else
        {
            (void)snprintf(out, out_size, "%s", input);
            out[out_size - 1u] = '\0';
        }
    }
}

static void vm_comm_set_error(char *error,
                              size_t error_size,
                              const char *message)
{
    vm_comm_copy_text(error, error_size, message);
}

static void vm_comm_clear_pending(vm_comm_t *comm)
{
    if (comm != NULL)
    {
        (void)memset(comm->pending, 0, sizeof(comm->pending));
    }
}

static void vm_comm_copy_variable(vm_comm_variable_store_t *out,
                                  const vm_comm_variable_t *input)
{
    if ((out != NULL) && (input != NULL))
    {
        (void)memset(out, 0, sizeof(*out));
        vm_comm_copy_text(out->name, sizeof(out->name), input->name);
        vm_comm_copy_text(out->type_name,
                          sizeof(out->type_name),
                          input->type_name);
        out->address = input->address;
        out->size = input->size;
        out->writable = input->writable;
        out->monitorable = input->monitorable;
        out->calibratable = input->calibratable;
        out->bit_field = input->bit_field;
        out->bit_offset = input->bit_offset;
        out->bit_size = input->bit_size;
    }
}

static void vm_comm_meta_from_variable(const vm_comm_variable_store_t *variable,
                                       vm_value_meta_t *meta)
{
    if ((variable != NULL) && (meta != NULL))
    {
        meta->name = variable->name;
        meta->type_name = variable->type_name;
        meta->size = variable->size;
        meta->bit_field = variable->bit_field;
        meta->bit_offset = variable->bit_offset;
        meta->bit_size = variable->bit_size;
    }
}

static vm_comm_pending_t *vm_comm_alloc_pending(vm_comm_t *comm,
                                                uint8_t sequence,
                                                vm_comm_pending_kind_t kind,
                                                const vm_comm_variable_store_t *var,
                                                const char *target_text)
{
    size_t index;
    vm_comm_pending_t *pending;

    pending = NULL;
    if ((comm != NULL) && (var != NULL))
    {
        for (index = 0u; index < VM_COMM_PENDING_MAX; ++index)
        {
            if (comm->pending[index].used == 0u)
            {
                pending = &comm->pending[index];
                (void)memset(pending, 0, sizeof(*pending));
                pending->used = 1u;
                pending->sequence = sequence;
                pending->kind = kind;
                pending->variable = *var;
                vm_comm_copy_text(pending->target_text,
                                  sizeof(pending->target_text),
                                  target_text);
                break;
            }
        }
    }

    return pending;
}

static vm_comm_pending_t *vm_comm_find_pending(vm_comm_t *comm,
                                               uint8_t sequence,
                                               uint32_t address,
                                               uint16_t size)
{
    size_t index;
    vm_comm_pending_t *fallback;
    vm_comm_pending_t *pending;

    fallback = NULL;
    pending = NULL;
    if (comm != NULL)
    {
        for (index = 0u; index < VM_COMM_PENDING_MAX; ++index)
        {
            if (comm->pending[index].used != 0u)
            {
                if (comm->pending[index].sequence == sequence)
                {
                    pending = &comm->pending[index];
                    break;
                }
                if ((fallback == NULL) &&
                    (comm->pending[index].variable.address == address) &&
                    ((size == 0u) || (comm->pending[index].variable.size == size)))
                {
                    fallback = &comm->pending[index];
                }
            }
        }
    }

    if (pending == NULL)
    {
        pending = fallback;
    }

    return pending;
}

static void vm_comm_free_pending(vm_comm_pending_t *pending)
{
    if (pending != NULL)
    {
        (void)memset(pending, 0, sizeof(*pending));
    }
}

static void vm_comm_emit(vm_comm_t *comm, const vm_comm_event_t *event)
{
    if ((comm != NULL) && (event != NULL) && (comm->callback != NULL))
    {
        comm->callback(comm->callback_context, event);
    }
}

static void vm_comm_fill_event_variable(vm_comm_event_t *event,
                                        const vm_comm_variable_store_t *variable)
{
    if ((event != NULL) && (variable != NULL))
    {
        event->address = variable->address;
        event->size = variable->size;
        vm_comm_copy_text(event->name, sizeof(event->name), variable->name);
    }
}

static uint8_t vm_comm_next_sequence(vm_comm_t *comm)
{
    uint8_t sequence;

    sequence = 0u;
    if (comm != NULL)
    {
        sequence = comm->sequence;
        comm->sequence = (uint8_t)(comm->sequence + 1u);
    }

    return sequence;
}

vm_status_t vm_comm_create(vm_comm_t **out)
{
    vm_comm_t *comm;

    if (out == NULL)
    {
        return VM_INVALID;
    }

    *out = NULL;
    comm = (vm_comm_t *)calloc(1u, sizeof(*comm));
    if (comm == NULL)
    {
        return VM_NOMEM;
    }

    comm->config.type = VM_COMM_DEVICE_SERIAL;
    comm->config.serial_baudrate = VM_COMM_DEFAULT_BAUDRATE;
    comm->config.serial_data_bits = VM_COMM_DEFAULT_DATA_BITS;
    comm->config.serial_stop_bits = VM_COMM_DEFAULT_STOP_BITS;
    *out = comm;
    return VM_OK;
}

void vm_comm_destroy(vm_comm_t *comm)
{
    if (comm != NULL)
    {
        vm_comm_disconnect(comm);
        free(comm);
    }
}

vm_status_t vm_comm_set_device_config(vm_comm_t *comm,
                                      const vm_comm_device_config_t *config)
{
    vm_status_t status;

    status = VM_INVALID;
    if ((comm != NULL) && (config != NULL))
    {
        comm->config = *config;
        vm_comm_copy_text(comm->serial_device,
                          sizeof(comm->serial_device),
                          config->serial_device);
        vm_comm_copy_text(comm->can_adapter,
                          sizeof(comm->can_adapter),
                          config->can_adapter);
        vm_comm_copy_text(comm->network_host,
                          sizeof(comm->network_host),
                          config->network_host);
        comm->config.serial_device = comm->serial_device;
        comm->config.can_adapter = comm->can_adapter;
        comm->config.network_host = comm->network_host;
        if (comm->config.serial_baudrate == 0u)
        {
            comm->config.serial_baudrate = VM_COMM_DEFAULT_BAUDRATE;
        }
        if (comm->config.serial_data_bits == 0u)
        {
            comm->config.serial_data_bits = VM_COMM_DEFAULT_DATA_BITS;
        }
        if (comm->config.serial_stop_bits == 0u)
        {
            comm->config.serial_stop_bits = VM_COMM_DEFAULT_STOP_BITS;
        }
        status = VM_OK;
    }

    return status;
}

vm_status_t vm_comm_connect(vm_comm_t *comm)
{
    vm_status_t status;

    if (comm == NULL)
    {
        return VM_INVALID;
    }

    vm_comm_disconnect(comm);
    switch (comm->config.type)
    {
        case VM_COMM_DEVICE_SERIAL:
            status = vm_comm_connect_serial(comm);
            break;
        case VM_COMM_DEVICE_CAN:
        case VM_COMM_DEVICE_CANFD:
        case VM_COMM_DEVICE_ETHERNET:
        default:
            status = VM_UNSUPPORTED;
            break;
    }

    if (status != VM_OK)
    {
        vm_comm_disconnect(comm);
    }

    return status;
}

void vm_comm_disconnect(vm_comm_t *comm)
{
    if (comm != NULL)
    {
        vm_serial_transport_destroy(comm->serial);
        comm->serial = NULL;
        (void)vm_router_destroy(comm->router);
        comm->router = NULL;
        vm_custom_parser_destroy(comm->parser);
        comm->parser = NULL;
        comm->sequence = 0u;
        vm_comm_clear_pending(comm);
    }
}

uint8_t vm_comm_is_connected(const vm_comm_t *comm)
{
    return (uint8_t)(((comm != NULL) && (comm->serial != NULL)) ? 1u : 0u);
}

void vm_comm_set_event_callback(vm_comm_t *comm,
                                vm_comm_event_fn callback,
                                void *context)
{
    if (comm != NULL)
    {
        comm->callback = callback;
        comm->callback_context = context;
    }
}

vm_status_t vm_comm_read_variable(vm_comm_t *comm,
                                  const vm_comm_variable_t *variable)
{
    vm_comm_variable_store_t stored;

    if ((comm == NULL) || (variable == NULL) || (variable->size == 0u))
    {
        return VM_INVALID;
    }

    vm_comm_copy_variable(&stored, variable);
    return vm_comm_send_read(comm, &stored, VM_COMM_PENDING_READ, NULL);
}

vm_status_t vm_comm_write_variable(vm_comm_t *comm,
                                   const vm_comm_variable_t *variable,
                                   const char *target_text,
                                   char *error,
                                   size_t error_size)
{
    vm_comm_variable_store_t stored;
    vm_value_meta_t meta;
    uint8_t data[VM_COMM_PAYLOAD_MAX];
    size_t written;
    vm_status_t status;

    if ((comm == NULL) || (variable == NULL) || (target_text == NULL) ||
        (variable->size == 0u))
    {
        vm_comm_set_error(error, error_size, "参数错误");
        return VM_INVALID;
    }
    if (vm_comm_is_connected(comm) == 0u)
    {
        vm_comm_set_error(error, error_size, "设备未连接");
        return VM_BUSY;
    }
    if (variable->size > VM_COMM_PAYLOAD_MAX)
    {
        vm_comm_set_error(error, error_size, "写入数据超过协议单帧长度");
        return VM_INVALID;
    }

    vm_comm_copy_variable(&stored, variable);
    if (stored.bit_field != 0u)
    {
        status = vm_comm_send_read(comm,
                                   &stored,
                                   VM_COMM_PENDING_BITFIELD_READ,
                                   target_text);
        if (status != VM_OK)
        {
            vm_comm_set_error(error, error_size, "位字段标定前读取原始值失败");
        }
        return status;
    }

    vm_comm_meta_from_variable(&stored, &meta);
    status = vm_value_encode(&meta,
                             NULL,
                             0u,
                             target_text,
                             data,
                             sizeof(data),
                             &written,
                             error,
                             error_size);
    if (status == VM_OK)
    {
        status = vm_comm_send_write_bytes(comm,
                                          &stored,
                                          data,
                                          (uint16_t)written);
        if (status != VM_OK)
        {
            vm_comm_set_error(error, error_size, "写入请求发送失败");
        }
    }

    return status;
}

vm_status_t vm_comm_poll(vm_comm_t *comm, uint8_t budget)
{
    uint8_t index;
    vm_status_t status;

    if ((comm == NULL) || (comm->serial == NULL))
    {
        return VM_BUSY;
    }

    status = VM_OK;
    for (index = 0u; index < budget; ++index)
    {
        status = vm_serial_transport_receive_once(comm->serial, 0u);
        if (status != VM_OK)
        {
            break;
        }
    }

    return status;
}

static vm_status_t vm_comm_connect_serial(vm_comm_t *comm)
{
    vm_serial_config_t serial_config;
    vm_route_t route;
    vm_status_t status;

    if (comm == NULL)
    {
        return VM_INVALID;
    }

    serial_config.device = comm->config.serial_device;
    serial_config.baudrate = comm->config.serial_baudrate;
    serial_config.data_bits = comm->config.serial_data_bits;
    serial_config.stop_bits = comm->config.serial_stop_bits;
    serial_config.parity = comm->config.serial_parity;
    serial_config.flow_control = comm->config.serial_flow_control;

    status = vm_serial_transport_create(&serial_config, &comm->serial);
    if (status == VM_OK)
    {
        status = vm_transport_open(vm_serial_transport_base(comm->serial));
    }
    if (status == VM_OK)
    {
        status = vm_custom_parser_create(&comm->parser);
    }
    if (status == VM_OK)
    {
        status = vm_router_create(&comm->router);
    }
    if (status == VM_OK)
    {
        status = vm_serial_transport_base(comm->serial)->ops->set_rx(
            vm_serial_transport_base(comm->serial),
            vm_comm_on_transport_packet,
            comm);
    }
    if (status == VM_OK)
    {
        route.route_id = VM_COMM_CUSTOM_SERIAL_ROUTE_ID;
        route.connection = VM_COMM_SERIAL_CONNECTION_ID;
        route.channel = VM_COMM_SERIAL_CHANNEL;
        route.rx_address = VM_COMM_SERIAL_ADDRESS;
        route.tx_address = VM_COMM_SERIAL_ADDRESS;
        route.protocol_receive = vm_comm_on_router_protocol_receive;
        route.transport_send = vm_comm_on_router_transport_send;
        route.protocol_context = comm;
        route.transport_context = comm;
        status = vm_router_bind(comm->router, &route);
    }
    if (status == VM_OK)
    {
        comm->sequence = 0u;
        vm_comm_clear_pending(comm);
    }

    return status;
}

static vm_status_t vm_comm_send_read(vm_comm_t *comm,
                                     const vm_comm_variable_store_t *variable,
                                     vm_comm_pending_kind_t kind,
                                     const char *target_text)
{
    uint8_t frame[VM_COMM_FRAME_MAX];
    uint8_t sequence;
    size_t written;
    vm_status_t status;

    if ((comm == NULL) || (comm->serial == NULL) || (comm->router == NULL) ||
        (comm->parser == NULL) || (variable == NULL))
    {
        return VM_BUSY;
    }
    if (variable->size > VM_COMM_PAYLOAD_MAX)
    {
        return VM_INVALID;
    }

    sequence = vm_comm_next_sequence(comm);
    status = vm_custom_make_read(sequence,
                                 variable->address,
                                 variable->size,
                                 frame,
                                 sizeof(frame),
                                 &written);
    if (status == VM_OK)
    {
        if (vm_comm_alloc_pending(comm,
                                  sequence,
                                  kind,
                                  variable,
                                  target_text) == NULL)
        {
            status = VM_BUSY;
        }
    }
    if (status == VM_OK)
    {
        status = vm_router_send(comm->router,
                                VM_COMM_CUSTOM_SERIAL_ROUTE_ID,
                                frame,
                                written);
    }

    return status;
}

static vm_status_t vm_comm_send_write_bytes(vm_comm_t *comm,
                                            const vm_comm_variable_store_t *var,
                                            const uint8_t *data,
                                            uint16_t size)
{
    uint8_t frame[VM_COMM_FRAME_MAX];
    uint8_t sequence;
    size_t written;
    vm_status_t status;

    if ((comm == NULL) || (comm->serial == NULL) || (comm->router == NULL) ||
        (comm->parser == NULL) || (var == NULL) ||
        ((data == NULL) && (size > 0u)))
    {
        return VM_BUSY;
    }
    if (size > VM_COMM_PAYLOAD_MAX)
    {
        return VM_INVALID;
    }

    sequence = vm_comm_next_sequence(comm);
    status = vm_custom_make_write(sequence,
                                  var->address,
                                  data,
                                  size,
                                  frame,
                                  sizeof(frame),
                                  &written);
    if (status == VM_OK)
    {
        if (vm_comm_alloc_pending(comm,
                                  sequence,
                                  VM_COMM_PENDING_WRITE,
                                  var,
                                  NULL) == NULL)
        {
            status = VM_BUSY;
        }
    }
    if (status == VM_OK)
    {
        status = vm_router_send(comm->router,
                                VM_COMM_CUSTOM_SERIAL_ROUTE_ID,
                                frame,
                                written);
    }

    return status;
}

static vm_status_t vm_comm_handle_read_response(vm_comm_t *comm,
                                                const vm_custom_message_t *msg,
                                                vm_comm_pending_t *pending)
{
    vm_value_meta_t meta;
    vm_comm_event_t event;
    uint8_t encoded[VM_COMM_PAYLOAD_MAX];
    size_t written;
    vm_status_t status;
    char error[VM_COMM_TEXT_MAX];

    if ((comm == NULL) || (msg == NULL))
    {
        return VM_INVALID;
    }
    if (pending == NULL)
    {
        return VM_OK;
    }

    if (pending->kind == VM_COMM_PENDING_BITFIELD_READ)
    {
        vm_comm_meta_from_variable(&pending->variable, &meta);
        status = vm_value_encode(&meta,
                                 msg->payload,
                                 msg->length,
                                 pending->target_text,
                                 encoded,
                                 sizeof(encoded),
                                 &written,
                                 error,
                                 sizeof(error));
        if (status == VM_OK)
        {
            vm_comm_variable_store_t variable;
            variable = pending->variable;
            vm_comm_free_pending(pending);
            status = vm_comm_send_write_bytes(comm,
                                              &variable,
                                              encoded,
                                              (uint16_t)written);
        }
        else
        {
            (void)memset(&event, 0, sizeof(event));
            event.type = VM_COMM_EVENT_ERROR;
            event.protocol_command = msg->command;
            vm_comm_fill_event_variable(&event, &pending->variable);
            vm_comm_copy_text(event.message, sizeof(event.message), error);
            vm_comm_emit(comm, &event);
            vm_comm_free_pending(pending);
        }
        return status;
    }

    (void)memset(&event, 0, sizeof(event));
    event.type = VM_COMM_EVENT_VALUE;
    event.protocol_command = msg->command;
    vm_comm_fill_event_variable(&event, &pending->variable);
    vm_comm_meta_from_variable(&pending->variable, &meta);
    status = vm_value_format(&meta,
                             msg->payload,
                             msg->length,
                             event.display_value,
                             sizeof(event.display_value));
    if (status != VM_OK)
    {
        vm_comm_copy_text(event.display_value, sizeof(event.display_value), "-");
    }
    if (vm_value_to_double(&meta,
                           msg->payload,
                           msg->length,
                           &event.numeric_value) == VM_OK)
    {
        event.has_numeric_value = 1u;
    }
    vm_comm_copy_text(event.message, sizeof(event.message), "读取成功");
    vm_comm_emit(comm, &event);
    vm_comm_free_pending(pending);

    return VM_OK;
}

static vm_status_t vm_comm_handle_write_response(vm_comm_t *comm,
                                                 const vm_custom_message_t *msg,
                                                 vm_comm_pending_t *pending)
{
    vm_comm_event_t event;
    vm_comm_variable_store_t fallback;

    if ((comm == NULL) || (msg == NULL))
    {
        return VM_INVALID;
    }

    (void)memset(&event, 0, sizeof(event));
    event.type = VM_COMM_EVENT_WRITE_DONE;
    event.protocol_command = msg->command;
    if (pending != NULL)
    {
        vm_comm_fill_event_variable(&event, &pending->variable);
        vm_comm_free_pending(pending);
    }
    else
    {
        (void)memset(&fallback, 0, sizeof(fallback));
        fallback.address = msg->address;
        fallback.size = msg->length;
        vm_comm_fill_event_variable(&event, &fallback);
    }
    vm_comm_copy_text(event.message, sizeof(event.message), "写入成功");
    vm_comm_emit(comm, &event);

    return VM_OK;
}

static vm_status_t vm_comm_handle_error(vm_comm_t *comm,
                                        const vm_custom_message_t *msg,
                                        vm_comm_pending_t *pending)
{
    vm_comm_event_t event;
    vm_comm_variable_store_t fallback;

    if ((comm == NULL) || (msg == NULL))
    {
        return VM_INVALID;
    }

    (void)memset(&event, 0, sizeof(event));
    event.type = VM_COMM_EVENT_ERROR;
    event.protocol_command = msg->command;
    event.error_code = (msg->length > 0u) ? msg->payload[0] : 0u;
    if (pending != NULL)
    {
        vm_comm_fill_event_variable(&event, &pending->variable);
        vm_comm_free_pending(pending);
    }
    else
    {
        (void)memset(&fallback, 0, sizeof(fallback));
        fallback.address = msg->address;
        fallback.size = msg->length;
        vm_comm_fill_event_variable(&event, &fallback);
    }
    (void)snprintf(event.message,
                   sizeof(event.message),
                   "协议错误：0x%02X",
                   (unsigned)event.error_code);
    event.message[sizeof(event.message) - 1u] = '\0';
    vm_comm_emit(comm, &event);

    return VM_OK;
}

static vm_status_t vm_comm_on_message(void *context,
                                      const vm_custom_message_t *message)
{
    vm_comm_t *comm;
    vm_comm_pending_t *pending;
    uint16_t length;
    vm_status_t status;

    comm = (vm_comm_t *)context;
    if ((comm == NULL) || (message == NULL))
    {
        return VM_INVALID;
    }

    length = message->length;
    pending = vm_comm_find_pending(comm,
                                   message->sequence,
                                   message->address,
                                   length);

    switch (message->command)
    {
        case VM_CUSTOM_READ_RESPONSE:
            status = vm_comm_handle_read_response(comm, message, pending);
            break;
        case VM_CUSTOM_WRITE_RESPONSE:
            status = vm_comm_handle_write_response(comm, message, pending);
            break;
        case VM_CUSTOM_ERROR:
            status = vm_comm_handle_error(comm, message, pending);
            break;
        default:
            status = VM_OK;
            break;
    }

    return status;
}

static vm_status_t vm_comm_on_router_protocol_receive(void *context,
                                                      const vm_frame_t *frame)
{
    vm_comm_t *comm;

    comm = (vm_comm_t *)context;
    if ((comm == NULL) || (comm->parser == NULL) || (frame == NULL))
    {
        return VM_INVALID;
    }

    return vm_custom_parser_feed(comm->parser,
                                 frame->data,
                                 frame->size,
                                 vm_comm_on_message,
                                 comm);
}

static vm_status_t vm_comm_on_router_transport_send(void *context,
                                                    const vm_frame_t *frame)
{
    vm_comm_t *comm;

    comm = (vm_comm_t *)context;
    if ((comm == NULL) || (comm->serial == NULL) || (frame == NULL))
    {
        return VM_INVALID;
    }

    return vm_transport_send(vm_serial_transport_base(comm->serial),
                             frame->data,
                             frame->size,
                             frame->channel);
}

static vm_status_t vm_comm_on_transport_packet(
    void *context,
    const vm_transport_packet_t *packet)
{
    vm_comm_t *comm;
    vm_frame_t frame;

    comm = (vm_comm_t *)context;
    if ((comm == NULL) || (comm->router == NULL) || (packet == NULL))
    {
        return VM_INVALID;
    }

    frame.connection = VM_COMM_SERIAL_CONNECTION_ID;
    frame.channel = packet->channel;
    frame.address = VM_COMM_SERIAL_ADDRESS;
    frame.data = packet->data;
    frame.size = packet->size;

    return vm_router_receive(comm->router, &frame);
}
