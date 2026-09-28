/*
 * 文件说明：通信模块 COM 层实现，负责连接生命周期、请求队列、协议收发和值编解码调度。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_communication.h"

#include "vm_custom_protocol.h"
#include "vm_router.h"
#include "vm_serial_transport.h"
#include "vm_value_codec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 常量说明：VM_COMM_SERIAL_CONNECTION_ID 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_SERIAL_CONNECTION_ID (1u)
/* 常量说明：VM_COMM_CUSTOM_SERIAL_ROUTE_ID 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_CUSTOM_SERIAL_ROUTE_ID (1u)
/* 常量说明：VM_COMM_SERIAL_CHANNEL 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_SERIAL_CHANNEL (0u)
/* 常量说明：VM_COMM_SERIAL_ADDRESS 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_SERIAL_ADDRESS (0u)
/* 常量说明：VM_COMM_FRAME_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_FRAME_MAX (1037u)
/* 常量说明：VM_COMM_PAYLOAD_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_PAYLOAD_MAX (1024u)
/* 常量说明：VM_COMM_PENDING_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_PENDING_MAX (32u)
/* 常量说明：VM_COMM_DEFAULT_DATA_BITS 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_DEFAULT_DATA_BITS (8u)
/* 常量说明：VM_COMM_DEFAULT_STOP_BITS 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_DEFAULT_STOP_BITS (1u)
/* 常量说明：VM_COMM_DEFAULT_BAUDRATE 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_COMM_DEFAULT_BAUDRATE (115200u)

/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
typedef enum
{
    VM_COMM_PENDING_NONE = 0,
    VM_COMM_PENDING_READ,
    VM_COMM_PENDING_WRITE,
    VM_COMM_PENDING_BITFIELD_READ
} vm_comm_pending_kind_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：name，变量名、页面名或节点名。 */
    char name[VM_COMM_NAME_MAX];
    /* 变量说明：type_name，变量类型名称。 */
    char type_name[VM_COMM_NAME_MAX];
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint32_t address;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint16_t size;
    /* 变量说明：writable，变量是否允许写入。 */
    uint8_t writable;
    /* 变量说明：monitorable，变量是否允许加入监控表。 */
    uint8_t monitorable;
    /* 变量说明：calibratable，变量是否允许加入标定表。 */
    uint8_t calibratable;
    /* 变量说明：bit_field，是否为位字段变量。 */
    uint8_t bit_field;
    /* 变量说明：bit_offset，位字段起始 bit 偏移。 */
    uint8_t bit_offset;
    /* 变量说明：bit_size，位字段 bit 宽度。 */
    uint8_t bit_size;
} vm_comm_variable_store_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：used，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t used;
    /* 变量说明：sequence，协议序号，用于匹配请求和响应。 */
    uint8_t sequence;
    /* 变量说明：kind，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_comm_pending_kind_t kind;
    /* 变量说明：variable，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_comm_variable_store_t variable;
    /* 变量说明：target_text，保存当前对象运行所需的状态、参数或缓存数据。 */
    char target_text[VM_COMM_TEXT_MAX];
} vm_comm_pending_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_comm
{
    /* 变量说明：serial，串口传输对象。 */
    vm_serial_transport_t *serial;
    /* 变量说明：router，内部路由对象。 */
    vm_router_t *router;
    /* 变量说明：parser，协议流式解析器。 */
    vm_custom_parser_t *parser;
    /* 变量说明：callback，事件回调函数。 */
    vm_comm_event_fn callback;
    /* 变量说明：callback_context，回调上下文。 */
    void *callback_context;
    /* 变量说明：config，设备配置缓存。 */
    vm_comm_device_config_t config;
    /* 变量说明：serial_device，保存当前对象运行所需的状态、参数或缓存数据。 */
    char serial_device[VM_COMM_TEXT_MAX];
    /* 变量说明：can_adapter，保存当前对象运行所需的状态、参数或缓存数据。 */
    char can_adapter[VM_COMM_TEXT_MAX];
    /* 变量说明：network_host，保存当前对象运行所需的状态、参数或缓存数据。 */
    char network_host[VM_COMM_TEXT_MAX];
    /* 变量说明：sequence，协议序号，用于匹配请求和响应。 */
    uint8_t sequence;
    /* 变量说明：pending，未完成请求数组。 */
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

/**
 * 函数说明：vm_comm_copy_text，复制文本或结构化数据。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。；out_size：函数输入参数，参与本函数的计算、查找或状态更新。；input：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
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

/**
 * 函数说明：vm_comm_set_error，执行本模块对应功能逻辑。
 * 输入：error：错误信息输出缓冲区。；error_size：错误信息缓冲区长度。；message：协议解析后的消息对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void vm_comm_set_error(char *error,
                              size_t error_size,
                              const char *message)
{
    vm_comm_copy_text(error, error_size, message);
}

/**
 * 函数说明：vm_comm_clear_pending，执行本模块对应功能逻辑。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void vm_comm_clear_pending(vm_comm_t *comm)
{
    if (comm != NULL)
    {
        (void)memset(comm->pending, 0, sizeof(comm->pending));
    }
}

/**
 * 函数说明：vm_comm_copy_variable，复制文本或结构化数据。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。；input：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
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

/**
 * 函数说明：vm_comm_meta_from_variable，执行本模块对应功能逻辑。
 * 输入：variable：变量描述信息，包含变量名、类型、地址、长度和权限。；meta：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
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

/**
 * 函数说明：vm_comm_alloc_pending，执行本模块对应功能逻辑。
 * 输入：comm：通信模块上下文指针。；sequence：函数输入参数，参与本函数的计算、查找或状态更新。；kind：函数输入参数，参与本函数的计算、查找或状态更新。；var：变量描述或变量缓存信息。；target_text：UI 输入的目标值字符串，通常为十进制文本。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
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

/**
 * 函数说明：vm_comm_find_pending，查找匹配对象。
 * 输入：comm：通信模块上下文指针。；sequence：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
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

/**
 * 函数说明：vm_comm_free_pending，执行本模块对应功能逻辑。
 * 输入：pending：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void vm_comm_free_pending(vm_comm_pending_t *pending)
{
    if (pending != NULL)
    {
        (void)memset(pending, 0, sizeof(*pending));
    }
}

/**
 * 函数说明：vm_comm_emit，执行本模块对应功能逻辑。
 * 输入：comm：通信模块上下文指针。；event：UI 或通信模块事件对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void vm_comm_emit(vm_comm_t *comm, const vm_comm_event_t *event)
{
    if ((comm != NULL) && (event != NULL) && (comm->callback != NULL))
    {
        comm->callback(comm->callback_context, event);
    }
}

/**
 * 函数说明：vm_comm_fill_event_variable，处理 UI 或通信事件。
 * 输入：event：UI 或通信模块事件对象。；variable：变量描述信息，包含变量名、类型、地址、长度和权限。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
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

/**
 * 函数说明：vm_comm_next_sequence，执行本模块对应功能逻辑。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
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

/**
 * 函数说明：vm_comm_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_destroy，销毁对象并释放相关资源。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_comm_destroy(vm_comm_t *comm)
{
    if (comm != NULL)
    {
        vm_comm_disconnect(comm);
        free(comm);
    }
}

/**
 * 函数说明：vm_comm_set_device_config，执行本模块对应功能逻辑。
 * 输入：comm：通信模块上下文指针。；config：设备连接参数或模块初始化参数。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_connect，建立连接并准备收发链路。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_disconnect，建立连接并准备收发链路。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
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
        /* 关键步骤：连接成功后重置协议序号和 pending 表，避免沿用上一次连接残留的请求状态。 */
        comm->sequence = 0u;
        vm_comm_clear_pending(comm);
    }
}

/**
 * 函数说明：vm_comm_is_connected，建立连接并准备收发链路。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 1 表示条件成立或状态有效，返回 0 表示条件不成立。
 */
uint8_t vm_comm_is_connected(const vm_comm_t *comm)
{
    return (uint8_t)(((comm != NULL) && (comm->serial != NULL)) ? 1u : 0u);
}

/**
 * 函数说明：vm_comm_set_event_callback，处理 UI 或通信事件。
 * 输入：comm：通信模块上下文指针。；callback：事件回调函数指针。；context：回调上下文指针，由调用方传入并在回调中原样返回。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
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

/**
 * 函数说明：vm_comm_read_variable，读取数据或发起读取请求。
 * 输入：comm：通信模块上下文指针。；variable：变量描述信息，包含变量名、类型、地址、长度和权限。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_write_variable，写入数据或发起标定请求。
 * 输入：comm：通信模块上下文指针。；variable：变量描述信息，包含变量名、类型、地址、长度和权限。；target_text：UI 输入的目标值字符串，通常为十进制文本。；error：错误信息输出缓冲区。；error_size：错误信息缓冲区长度。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_poll，轮询处理异步收发事件。
 * 输入：comm：通信模块上下文指针。；budget：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_connect_serial，建立连接并准备收发链路。
 * 输入：comm：通信模块上下文指针。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t vm_comm_connect_serial(vm_comm_t *comm)
{
    vm_serial_config_t serial_config;
    vm_route_t route;
    vm_status_t status;

    if (comm == NULL)
    {
        return VM_INVALID;
    }

    /* 关键步骤：把 COM 层缓存的串口配置转换为底层 serial transport 使用的配置结构。 */
    serial_config.device = comm->config.serial_device;
    serial_config.baudrate = comm->config.serial_baudrate;
    serial_config.data_bits = comm->config.serial_data_bits;
    serial_config.stop_bits = comm->config.serial_stop_bits;
    serial_config.parity = comm->config.serial_parity;
    serial_config.flow_control = comm->config.serial_flow_control;

    /* 关键步骤：按顺序创建串口对象、打开设备、创建协议解析器和路由器，任一步失败都停止后续初始化。 */
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
        /* 关键步骤：绑定自定义协议的串口路由，后续读写请求统一通过 route_id 找到发送通道。 */
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

/**
 * 函数说明：vm_comm_send_read，读取数据或发起读取请求。
 * 输入：comm：通信模块上下文指针。；variable：变量描述信息，包含变量名、类型、地址、长度和权限。；kind：函数输入参数，参与本函数的计算、查找或状态更新。；target_text：UI 输入的目标值字符串，通常为十进制文本。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

    /* 关键步骤：为读请求分配新的协议序号，并把变量地址和长度编码成自定义协议读帧。 */
    sequence = vm_comm_next_sequence(comm);
    status = vm_custom_make_read(sequence,
                                 variable->address,
                                 variable->size,
                                 frame,
                                 sizeof(frame),
                                 &written);
    if (status == VM_OK)
    {
        /* 关键步骤：发送前登记 pending 请求，响应回来后用 sequence 找回变量元数据和请求类型。 */
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
        /* 关键步骤：通过路由层发送已编码的协议帧，COM 层不直接操作串口发送细节。 */
        status = vm_router_send(comm->router,
                                VM_COMM_CUSTOM_SERIAL_ROUTE_ID,
                                frame,
                                written);
    }

    return status;
}

/**
 * 函数说明：vm_comm_send_write_bytes，写入数据或发起标定请求。
 * 输入：comm：通信模块上下文指针。；var：变量描述或变量缓存信息。；data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

    /* 关键步骤：写请求同样使用独立序号，保证写确认能和当前标定变量对应。 */
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

/**
 * 函数说明：vm_comm_handle_read_response，读取数据或发起读取请求。
 * 输入：comm：通信模块上下文指针。；msg：协议解析后的消息对象。；pending：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

    /* 关键步骤：位字段写入需要先读出整个存储单元，再只替换目标 bit，避免破坏同一字节/字中的其它位。 */
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
            /* 关键步骤：所有协议结果最后统一转换成 COM 事件上报 UI，UI 不需要理解底层协议帧。 */
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
    /* 关键步骤：读响应先格式化为 UI 显示用十进制文本，再尝试转换为曲线绘图用 double。 */
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

/**
 * 函数说明：vm_comm_handle_write_response，写入数据或发起标定请求。
 * 输入：comm：通信模块上下文指针。；msg：协议解析后的消息对象。；pending：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_handle_error，执行本模块对应功能逻辑。
 * 输入：comm：通信模块上下文指针。；msg：协议解析后的消息对象。；pending：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_on_message，执行本模块对应功能逻辑。
 * 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。；message：协议解析后的消息对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_on_router_protocol_receive，执行本模块对应功能逻辑。
 * 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。；frame：协议帧或传输帧数据。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_on_router_transport_send，执行本模块对应功能逻辑。
 * 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。；frame：协议帧或传输帧数据。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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

/**
 * 函数说明：vm_comm_on_transport_packet，执行本模块对应功能逻辑。
 * 输入：context：回调上下文指针，由调用方传入并在回调中原样返回。；packet：底层传输收到的数据包。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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
