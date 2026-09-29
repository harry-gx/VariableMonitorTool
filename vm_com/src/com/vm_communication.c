/*
 * 文件说明：通信模块 COM 层实现，向 UI 汇总连接、变量读取、变量标定和事件回调能力。
 * 所属模块：通信模块 / COM 层。
 * 设计要点：COM 层不直接操作串口驱动和协议拆包，底层链路统一走 MCAL->IF->PduR->Services。
 */

#include "vm_communication.h"

#include "vm_service.h"
#include "vm_mcal_serial.h"
#include "vm_pdur.h"
#include "vm_serial_if.h"
#include "vm_value_codec.h"

#include <stdio.h>
#include <string.h>

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
    vm_serial_if_t *serial_if;
    vm_mcal_serial_device_t *serial_device;
    vm_pdur_t *pdur;
    vm_pdur_lower_route_t *serial_route;
    vm_service_manager_t *services;
    vm_comm_event_fn callback;
    void *callback_context;
    vm_comm_device_config_t config;
    char serial_device_name[VM_COMM_TEXT_MAX];
    char can_adapter[VM_COMM_TEXT_MAX];
    char network_host[VM_COMM_TEXT_MAX];
    uint8_t sequence;
    vm_comm_pending_t pending[VM_COMM_PENDING_MAX];
};

/* 变量说明：COM 静态通信对象，UI 当前只需要一个通信实例。 */
static vm_comm_t g_vm_comm_instance;
/* 变量说明：COM 静态对象占用标志，防止重复创建同一个通信实例。 */
static uint8_t g_vm_comm_instance_used;

/* 变量说明：串口下层路由静态节点，删除下方路由表项即可禁用串口通道。 */
static vm_pdur_lower_route_t g_vm_comm_serial_route;

/*
 * 变量说明：COM 层显式下层路由表。
 * 修改说明：删除某个表项后，对应底层设备不会注册到 PduR，协议无法通过该设备收发。
 */
static vm_pdur_lower_route_t *const g_vm_comm_lower_route_table[] =
{
    &g_vm_comm_serial_route
};

static vm_status_t vm_comm_connect_serial(vm_comm_t *comm);
static vm_status_t vm_comm_register_lower_routes(vm_comm_t *comm);
static vm_status_t vm_comm_serial_route_read(void *lower_layer,
                                             void *lower_device,
                                             uint8_t *buffer,
                                             size_t capacity,
                                             size_t *read_size);
static vm_status_t vm_comm_serial_route_write(void *lower_layer,
                                              void *lower_device,
                                              const uint8_t *data,
                                              size_t size);
static vm_status_t vm_comm_send_request(vm_comm_t *comm,
                                        const vm_comm_variable_store_t *variable,
                                        uint8_t command,
                                        vm_comm_pending_kind_t kind,
                                        const uint8_t *payload,
                                        uint16_t length,
                                        const char *target_text);
static vm_status_t vm_comm_handle_read_response(vm_comm_t *comm,
                                                const vm_pdur_message_t *msg,
                                                vm_comm_pending_t *pending);
static vm_status_t vm_comm_handle_write_response(vm_comm_t *comm,
                                                 const vm_pdur_message_t *msg,
                                                 vm_comm_pending_t *pending);
static vm_status_t vm_comm_handle_error(vm_comm_t *comm,
                                        const vm_pdur_message_t *msg,
                                        vm_comm_pending_t *pending);
static vm_status_t vm_comm_on_message(void *context,
                                      const vm_pdur_message_t *message);

/**
 * 函数说明：安全复制文本。
 * 输入：out，输出缓冲区；out_size，缓冲区容量；input，待复制字符串。
 * 输出：out 写入以 '\0' 结尾的字符串。
 * 返回：无。
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
 * 函数说明：写入错误提示文本。
 * 输入：error，错误缓冲区；error_size，缓冲区容量；message，错误文本。
 * 输出：error 写入可展示给 UI 的错误原因。
 * 返回：无。
 */
static void vm_comm_set_error(char *error,
                              size_t error_size,
                              const char *message)
{
    vm_comm_copy_text(error, error_size, message);
}

/**
 * 函数说明：清空所有 pending 请求。
 * 输入：comm，COM 控制块。
 * 输出：pending 表全部置空。
 * 返回：无。
 */
static void vm_comm_clear_pending(vm_comm_t *comm)
{
    if (comm != NULL)
    {
        (void)memset(comm->pending, 0, sizeof(comm->pending));
    }
}

/**
 * 函数说明：把 UI 变量描述复制为 pending 可长期持有的内部缓存。
 * 输入：out，输出变量缓存；input，UI 变量描述。
 * 输出：out 保存变量名、类型、地址、长度和权限。
 * 返回：无。
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
 * 函数说明：把变量缓存转换为值编解码元数据。
 * 输入：variable，变量缓存；meta，值编解码元数据。
 * 输出：meta 指向 variable 内部字符串并携带类型、长度、位字段信息。
 * 返回：无。
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
 * 函数说明：分配一个 pending 槽位。
 * 输入：comm，COM 控制块；sequence，协议序号；kind，请求类型；var，变量信息；
 *       target_text，位字段目标值，可为 NULL。
 * 输出：pending 表新增一条记录。
 * 返回：成功返回 pending 指针，失败返回 NULL。
 */
static vm_comm_pending_t *vm_comm_alloc_pending(
    vm_comm_t *comm,
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
 * 函数说明：根据协议序号、地址和长度查找 pending 请求。
 * 输入：comm，COM 控制块；sequence，响应序号；address，响应地址；size，响应长度。
 * 输出：无。
 * 返回：找到返回 pending 指针，未找到返回 NULL。
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
                    ((size == 0u) ||
                     (comm->pending[index].variable.size == size)))
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
 * 函数说明：释放一个 pending 槽位。
 * 输入：pending，待释放请求记录。
 * 输出：pending 内容清零。
 * 返回：无。
 */
static void vm_comm_free_pending(vm_comm_pending_t *pending)
{
    if (pending != NULL)
    {
        (void)memset(pending, 0, sizeof(*pending));
    }
}

/**
 * 函数说明：向 UI 上报事件。
 * 输入：comm，COM 控制块；event，事件对象。
 * 输出：若 UI 注册回调，则回调被调用。
 * 返回：无。
 */
static void vm_comm_emit(vm_comm_t *comm, const vm_comm_event_t *event)
{
    if ((comm != NULL) && (event != NULL) && (comm->callback != NULL))
    {
        comm->callback(comm->callback_context, event);
    }
}

/**
 * 函数说明：把变量基本信息填充到 UI 事件。
 * 输入：event，待填充事件；variable，变量缓存。
 * 输出：event 填入地址、长度和名称。
 * 返回：无。
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
 * 函数说明：生成下一个自定义协议序号。
 * 输入：comm，COM 控制块。
 * 输出：comm->sequence 自增。
 * 返回：本次请求使用的序号。
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

vm_status_t vm_comm_create(vm_comm_t **out)
{
    vm_comm_t *comm;
    vm_status_t status;

    if (out == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        *out = NULL;
        if (g_vm_comm_instance_used != 0u)
        {
            status = VM_BUSY;
        }
        else
        {
            comm = &g_vm_comm_instance;
            (void)memset(comm, 0, sizeof(*comm));
            comm->config.type = VM_COMM_DEVICE_SERIAL;
            comm->config.serial_baudrate = VM_COMM_DEFAULT_BAUDRATE;
            comm->config.serial_data_bits = VM_COMM_DEFAULT_DATA_BITS;
            comm->config.serial_stop_bits = VM_COMM_DEFAULT_STOP_BITS;
            g_vm_comm_instance_used = 1u;
            *out = comm;
            status = VM_OK;
        }
    }

    return status;
}

void vm_comm_destroy(vm_comm_t *comm)
{
    if (comm == &g_vm_comm_instance)
    {
        vm_comm_disconnect(comm);
        (void)memset(comm, 0, sizeof(*comm));
        g_vm_comm_instance_used = 0u;
    }
}

vm_status_t vm_comm_set_device_config(vm_comm_t *comm,
                                      const vm_comm_device_config_t *config)
{
    vm_status_t status;

    if ((comm == NULL) || (config == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        comm->config = *config;
        vm_comm_copy_text(comm->serial_device_name,
                          sizeof(comm->serial_device_name),
                          config->serial_device);
        vm_comm_copy_text(comm->can_adapter,
                          sizeof(comm->can_adapter),
                          config->can_adapter);
        vm_comm_copy_text(comm->network_host,
                          sizeof(comm->network_host),
                          config->network_host);
        comm->config.serial_device = comm->serial_device_name;
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
        status = VM_INVALID;
    }
    else
    {
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
    }

    return status;
}

void vm_comm_disconnect(vm_comm_t *comm)
{
    if (comm != NULL)
    {
        vm_service_manager_destroy(comm->services);
        comm->services = NULL;

        if (comm->pdur != NULL)
        {
            vm_pdur_unregister_com_route(comm->pdur);
        }

        if ((comm->pdur != NULL) && (comm->serial_route != NULL))
        {
            (void)vm_pdur_unregister_lower_route(comm->pdur,
                                                 comm->serial_route);
            comm->serial_route = NULL;
        }

        vm_pdur_destroy(comm->pdur);
        comm->pdur = NULL;

        if ((comm->serial_if != NULL) && (comm->serial_device != NULL))
        {
            (void)vm_serial_if_unregister_device(comm->serial_if,
                                                 comm->serial_device);
            comm->serial_device = NULL;
        }

        vm_serial_if_destroy(comm->serial_if);
        comm->serial_if = NULL;

        comm->sequence = 0u;
        vm_comm_clear_pending(comm);
    }
}

uint8_t vm_comm_is_connected(const vm_comm_t *comm)
{
    uint8_t connected;

    connected = 0u;
    if ((comm != NULL) && (comm->serial_if != NULL) &&
        (comm->serial_device != NULL) && (comm->serial_route != NULL) &&
        (comm->services != NULL))
    {
        connected = vm_serial_if_is_open(comm->serial_if,
                                         comm->serial_device);
    }

    return connected;
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
    vm_status_t status;

    if ((comm == NULL) || (variable == NULL) || (variable->size == 0u))
    {
        status = VM_INVALID;
    }
    else
    {
        vm_comm_copy_variable(&stored, variable);
        status = vm_comm_send_request(comm,
                                      &stored,
                                      VM_PDUR_COMMAND_READ,
                                      VM_COMM_PENDING_READ,
                                      NULL,
                                      stored.size,
                                      NULL);
    }

    return status;
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
        status = VM_INVALID;
    }
    else if (vm_comm_is_connected(comm) == 0u)
    {
        vm_comm_set_error(error, error_size, "设备未连接");
        status = VM_BUSY;
    }
    else if (variable->size > VM_COMM_PAYLOAD_MAX)
    {
        vm_comm_set_error(error, error_size, "写入数据超过协议单帧长度");
        status = VM_INVALID;
    }
    else
    {
        vm_comm_copy_variable(&stored, variable);
        if (stored.bit_field != 0u)
        {
            status = vm_comm_send_request(comm,
                                           &stored,
                                           VM_PDUR_COMMAND_READ,
                                           VM_COMM_PENDING_BITFIELD_READ,
                                           NULL,
                                           stored.size,
                                           target_text);
            if (status != VM_OK)
            {
                vm_comm_set_error(error,
                                  error_size,
                                  "位字段标定前读取原始值失败");
            }
        }
        else
        {
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
                status = vm_comm_send_request(comm,
                                              &stored,
                                              VM_PDUR_COMMAND_WRITE,
                                              VM_COMM_PENDING_WRITE,
                                              data,
                                              (uint16_t)written,
                                              NULL);
                if (status != VM_OK)
                {
                    vm_comm_set_error(error, error_size, "写入请求发送失败");
                }
            }
        }
    }

    return status;
}

vm_status_t vm_comm_poll(vm_comm_t *comm, uint8_t budget)
{
    vm_status_t status;

    if ((comm == NULL) || (comm->pdur == NULL) ||
        (comm->serial_route == NULL) || (comm->services == NULL))
    {
        status = VM_BUSY;
    }
    else
    {
        status = vm_pdur_poll(comm->pdur, budget);
    }

    return status;
}

/**
 * 函数说明：PduR 串口下层路由读取适配函数。
 * 输入：lower_layer，串口 IF 控制块；lower_device，串口设备；buffer，接收缓冲区；capacity，缓冲区容量。
 * 输出：read_size 返回实际读取字节数。
 * 返回：VM_OK 表示读到数据，VM_NOT_FOUND 表示暂无数据，其它状态码表示读取失败。
 */
static vm_status_t vm_comm_serial_route_read(void *lower_layer,
                                             void *lower_device,
                                             uint8_t *buffer,
                                             size_t capacity,
                                             size_t *read_size)
{
    return vm_serial_if_read((vm_serial_if_t *)lower_layer,
                             (vm_mcal_serial_device_t *)lower_device,
                             buffer,
                             capacity,
                             read_size);
}

/**
 * 函数说明：PduR 串口下层路由发送适配函数。
 * 输入：lower_layer，串口 IF 控制块；lower_device，串口设备；data，待发送数据；size，待发送长度。
 * 输出：数据通过串口 IF 发送到设备。
 * 返回：VM_OK 表示发送成功，其它状态码表示发送失败。
 */
static vm_status_t vm_comm_serial_route_write(void *lower_layer,
                                              void *lower_device,
                                              const uint8_t *data,
                                              size_t size)
{
    return vm_serial_if_write((vm_serial_if_t *)lower_layer,
                              (vm_mcal_serial_device_t *)lower_device,
                              data,
                              size);
}

/**
 * 函数说明：按照显式下层路由表把底层设备节点挂接到 PduR。
 * 输入：comm，COM 控制块。
 * 输出：表中启用的下层路由节点被注册到 PduR；删除表项即可禁用对应底层设备。
 * 返回：VM_OK 表示全部注册成功，其它状态码表示路由配置或注册失败。
 */
static vm_status_t vm_comm_register_lower_routes(vm_comm_t *comm)
{
    size_t index;
    vm_pdur_lower_route_t *route;
    vm_status_t status;

    if ((comm == NULL) || (comm->pdur == NULL) ||
        (comm->serial_if == NULL) || (comm->serial_device == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        (void)memset(&g_vm_comm_serial_route, 0, sizeof(g_vm_comm_serial_route));
        g_vm_comm_serial_route.config.device_type = VM_PDUR_DEVICE_SERIAL;
        g_vm_comm_serial_route.config.channel = VM_PDUR_CHANNEL_DEFAULT;
        g_vm_comm_serial_route.config.lower_layer = comm->serial_if;
        g_vm_comm_serial_route.config.lower_device = comm->serial_device;
        g_vm_comm_serial_route.config.read = vm_comm_serial_route_read;
        g_vm_comm_serial_route.config.write = vm_comm_serial_route_write;

        status = VM_NOT_FOUND;
        for (index = 0u;
             (index < (sizeof(g_vm_comm_lower_route_table) /
                       sizeof(g_vm_comm_lower_route_table[0]))) &&
             ((status == VM_OK) || (status == VM_NOT_FOUND));
             ++index)
        {
            route = g_vm_comm_lower_route_table[index];
            if (route == NULL)
            {
                status = VM_INVALID;
            }
            else
            {
                status = vm_pdur_register_lower_route(comm->pdur, route);
                if (status == VM_OK)
                {
                    comm->serial_route = route;
                }
            }
        }
    }

    return status;
}

/**
 * 函数说明：连接串口并搭建串口自定义协议栈。
 * 输入：comm，COM 控制块。
 * 输出：创建 IF 控制块、注册并打开串口设备、创建 PduR、注册下层路由并创建 Services。
 * 返回：VM_OK 表示完整链路搭建成功，其它状态码表示某一层初始化失败。
 */
static vm_status_t vm_comm_connect_serial(vm_comm_t *comm)
{
    vm_mcal_serial_config_t serial_config;
    vm_status_t status;

    if (comm == NULL)
    {
        status = VM_INVALID;
    }
    else
    {
        (void)memset(&serial_config, 0, sizeof(serial_config));
        vm_comm_copy_text(serial_config.device_name,
                          sizeof(serial_config.device_name),
                          comm->config.serial_device);
        serial_config.baudrate = comm->config.serial_baudrate;
        serial_config.data_bits = comm->config.serial_data_bits;
        serial_config.stop_bits = comm->config.serial_stop_bits;
        serial_config.parity = comm->config.serial_parity;
        serial_config.flow_control = comm->config.serial_flow_control;

        status = vm_serial_if_create(&comm->serial_if);
        if (status == VM_OK)
        {
            status = vm_serial_if_register_device(comm->serial_if,
                                                  &serial_config,
                                                  &comm->serial_device);
        }
        if (status == VM_OK)
        {
            status = vm_serial_if_open(comm->serial_if,
                                       comm->serial_device);
        }
        if (status == VM_OK)
        {
            status = vm_pdur_create(&comm->pdur);
        }
        if (status == VM_OK)
        {
            vm_pdur_com_config_t com_config;

            (void)memset(&com_config, 0, sizeof(com_config));
            com_config.indication = vm_comm_on_message;
            com_config.context = comm;
            status = vm_pdur_register_com_route(comm->pdur, &com_config);
        }
        if (status == VM_OK)
        {
            status = vm_comm_register_lower_routes(comm);
        }
        if (status == VM_OK)
        {
            status = vm_service_manager_create(comm->pdur,
                                               VM_PDUR_DEVICE_SERIAL,
                                               VM_PDUR_CHANNEL_DEFAULT,
                                               &comm->services);
        }
        if (status == VM_OK)
        {
            comm->sequence = 0u;
            vm_comm_clear_pending(comm);
        }
    }

    return status;
}

/**
 * 函数说明：通过 PduR 向协议服务发送变量读写请求。
 * 输入：comm，COM 控制块；variable，变量缓存；command，读写命令；kind，pending 类型；
 *       payload，写入负载；length，读写字节数；target_text，位字段目标值。
 * 输出：pending 表记录请求，PduR 将请求路由到协议服务。
 * 返回：VM_OK 表示发送成功，其它状态码表示未连接、参数错误或发送失败。
 */
static vm_status_t vm_comm_send_request(vm_comm_t *comm,
                                        const vm_comm_variable_store_t *variable,
                                        uint8_t command,
                                        vm_comm_pending_kind_t kind,
                                        const uint8_t *payload,
                                        uint16_t length,
                                        const char *target_text)
{
    vm_pdur_message_t message;
    uint8_t sequence;
    vm_comm_pending_t *pending;
    vm_status_t status;

    if ((comm == NULL) || (comm->services == NULL) ||
        (variable == NULL) ||
        ((command == VM_PDUR_COMMAND_WRITE) && (payload == NULL) &&
         (length > 0u)))
    {
        status = VM_BUSY;
    }
    else if ((length > VM_COMM_PAYLOAD_MAX) ||
             ((command != VM_PDUR_COMMAND_READ) &&
              (command != VM_PDUR_COMMAND_WRITE)))
    {
        status = VM_INVALID;
    }
    else
    {
        sequence = vm_comm_next_sequence(comm);
        pending = vm_comm_alloc_pending(comm,
                                        sequence,
                                        kind,
                                        variable,
                                        target_text);
        if (pending == NULL)
        {
            status = VM_BUSY;
        }
        else
        {
            (void)memset(&message, 0, sizeof(message));
            message.device_type = VM_PDUR_DEVICE_SERIAL;
            message.service_id = VM_PDUR_SERVICE_CUSTOM;
            message.channel = VM_PDUR_CHANNEL_DEFAULT;
            message.command = command;
            message.sequence = sequence;
            message.address = variable->address;
            message.payload = payload;
            message.length = length;
            status = vm_pdur_service_request(comm->pdur, &message);
            if (status != VM_OK)
            {
                vm_comm_free_pending(pending);
            }
        }
    }

    return status;
}

/**
 * 函数说明：处理读响应。
 * 输入：comm，COM 控制块；msg，自定义协议消息；pending，匹配到的请求。
 * 输出：普通读响应上报 VALUE 事件；位字段预读响应继续发送写请求。
 * 返回：VM_OK 表示处理完成。
 */
static vm_status_t vm_comm_handle_read_response(vm_comm_t *comm,
                                                const vm_pdur_message_t *msg,
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
        status = VM_INVALID;
    }
    else if (pending == NULL)
    {
        status = VM_OK;
    }
    else if (pending->kind == VM_COMM_PENDING_BITFIELD_READ)
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
            status = vm_comm_send_request(comm,
                                          &variable,
                                          VM_PDUR_COMMAND_WRITE,
                                          VM_COMM_PENDING_WRITE,
                                          encoded,
                                          (uint16_t)written,
                                          NULL);
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
    }
    else
    {
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
            vm_comm_copy_text(event.display_value,
                              sizeof(event.display_value),
                              "-");
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
        status = VM_OK;
    }

    return status;
}

/**
 * 函数说明：处理写响应。
 * 输入：comm，COM 控制块；msg，自定义协议消息；pending，匹配到的请求。
 * 输出：向 UI 上报 WRITE_DONE 事件。
 * 返回：VM_OK 表示处理完成。
 */
static vm_status_t vm_comm_handle_write_response(vm_comm_t *comm,
                                                 const vm_pdur_message_t *msg,
                                                 vm_comm_pending_t *pending)
{
    vm_comm_event_t event;
    vm_comm_variable_store_t fallback;
    vm_status_t status;

    if ((comm == NULL) || (msg == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
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
        status = VM_OK;
    }

    return status;
}

/**
 * 函数说明：处理协议错误响应。
 * 输入：comm，COM 控制块；msg，自定义协议消息；pending，匹配到的请求。
 * 输出：向 UI 上报 ERROR 事件，并释放 pending。
 * 返回：VM_OK 表示处理完成。
 */
static vm_status_t vm_comm_handle_error(vm_comm_t *comm,
                                        const vm_pdur_message_t *msg,
                                        vm_comm_pending_t *pending)
{
    vm_comm_event_t event;
    vm_comm_variable_store_t fallback;
    vm_status_t status;

    if ((comm == NULL) || (msg == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
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
        status = VM_OK;
    }

    return status;
}

/**
 * 函数说明：PduR 转发的协议逻辑消息回调。
 * 输入：context，COM 控制块；message，PduR 上报的协议逻辑消息。
 * 输出：根据 command 分派到读响应、写响应或错误响应处理函数。
 * 返回：VM_OK 表示消息处理完成。
 */
static vm_status_t vm_comm_on_message(void *context,
                                      const vm_pdur_message_t *message)
{
    vm_comm_t *comm;
    vm_comm_pending_t *pending;
    uint16_t length;
    vm_status_t status;

    comm = (vm_comm_t *)context;
    if ((comm == NULL) || (message == NULL))
    {
        status = VM_INVALID;
    }
    else
    {
        length = message->length;
        pending = vm_comm_find_pending(comm,
                                       message->sequence,
                                       message->address,
                                       length);

        switch (message->command)
        {
            case VM_PDUR_COMMAND_READ_RESPONSE:
                status = vm_comm_handle_read_response(comm, message, pending);
                break;

            case VM_PDUR_COMMAND_WRITE_RESPONSE:
                status = vm_comm_handle_write_response(comm, message, pending);
                break;

            case VM_PDUR_COMMAND_ERROR:
                status = vm_comm_handle_error(comm, message, pending);
                break;

            default:
                status = VM_OK;
                break;
        }
    }

    return status;
}
