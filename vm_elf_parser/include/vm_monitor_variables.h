#ifndef VM_MONITOR_VARIABLES_H
#define VM_MONITOR_VARIABLES_H

#include <stddef.h>
#include <stdint.h>

#include "vm_status.h"

VM_BEGIN

#define VM_MONITOR_NAME_MAX (256u)
#define VM_MONITOR_TYPE_NAME_MAX (96u)
#define VM_MONITOR_ERROR_MAX (256u)

typedef struct vm_monitor_variable_list vm_monitor_variable_list_t;

typedef struct
{
    char name[VM_MONITOR_NAME_MAX];
    char type_name[VM_MONITOR_TYPE_NAME_MAX];
    uint64_t address;
    uint64_t size;
    uint8_t writable;
    uint8_t monitorable;
    uint8_t calibratable;
    uint8_t bit_field;
    uint8_t bit_offset;
    uint8_t bit_size;
} vm_monitor_variable_t;

vm_status_t vm_monitor_variables_load(const char *path,
                                      vm_monitor_variable_list_t **out,
                                      char *error,
                                      size_t error_size);

size_t vm_monitor_variable_count(const vm_monitor_variable_list_t *list);

vm_status_t vm_monitor_variable_at(const vm_monitor_variable_list_t *list,
                                   size_t index,
                                   vm_monitor_variable_t *out);

void vm_monitor_variable_list_destroy(vm_monitor_variable_list_t *list);

VM_END

#endif
