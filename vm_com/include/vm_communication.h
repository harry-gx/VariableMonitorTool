#ifndef VM_COMMUNICATION_H
#define VM_COMMUNICATION_H

#include "vm_status.h"

VM_BEGIN

#define VM_COMM_NAME_MAX (128u)
#define VM_COMM_TEXT_MAX (128u)

typedef struct vm_comm vm_comm_t;

typedef enum
{
    VM_COMM_DEVICE_SERIAL = 0,
    VM_COMM_DEVICE_CAN,
    VM_COMM_DEVICE_CANFD,
    VM_COMM_DEVICE_ETHERNET
} vm_comm_device_type_t;

typedef enum
{
    VM_COMM_EVENT_VALUE = 1,
    VM_COMM_EVENT_WRITE_DONE,
    VM_COMM_EVENT_ERROR
} vm_comm_event_type_t;

typedef struct
{
    vm_comm_device_type_t type;
    const char *serial_device;
    uint32_t serial_baudrate;
    uint8_t serial_data_bits;
    uint8_t serial_stop_bits;
    uint8_t serial_parity;
    uint8_t serial_flow_control;
    const char *can_adapter;
    uint32_t can_channel;
    uint32_t can_baudrate;
    uint32_t canfd_data_baudrate;
    const char *network_host;
    uint16_t network_port;
} vm_comm_device_config_t;

typedef struct
{
    const char *name;
    const char *type_name;
    uint32_t address;
    uint16_t size;
    uint8_t writable;
    uint8_t monitorable;
    uint8_t calibratable;
    uint8_t bit_field;
    uint8_t bit_offset;
    uint8_t bit_size;
} vm_comm_variable_t;

typedef struct
{
    vm_comm_event_type_t type;
    uint32_t address;
    uint16_t size;
    uint8_t protocol_command;
    uint8_t error_code;
    uint8_t has_numeric_value;
    double numeric_value;
    char name[VM_COMM_NAME_MAX];
    char display_value[VM_COMM_TEXT_MAX];
    char message[VM_COMM_TEXT_MAX];
} vm_comm_event_t;

typedef void (*vm_comm_event_fn)(void *context, const vm_comm_event_t *event);

vm_status_t vm_comm_create(vm_comm_t **out);
void vm_comm_destroy(vm_comm_t *comm);
vm_status_t vm_comm_set_device_config(vm_comm_t *comm,
                                      const vm_comm_device_config_t *config);
vm_status_t vm_comm_connect(vm_comm_t *comm);
void vm_comm_disconnect(vm_comm_t *comm);
uint8_t vm_comm_is_connected(const vm_comm_t *comm);
void vm_comm_set_event_callback(vm_comm_t *comm,
                                vm_comm_event_fn callback,
                                void *context);
vm_status_t vm_comm_read_variable(vm_comm_t *comm,
                                  const vm_comm_variable_t *variable);
vm_status_t vm_comm_write_variable(vm_comm_t *comm,
                                   const vm_comm_variable_t *variable,
                                   const char *target_text,
                                   char *error,
                                   size_t error_size);
vm_status_t vm_comm_poll(vm_comm_t *comm, uint8_t budget);

VM_END

#endif
