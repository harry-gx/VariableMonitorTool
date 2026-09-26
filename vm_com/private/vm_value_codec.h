#ifndef VM_VALUE_CODEC_H
#define VM_VALUE_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "vm_status.h"

VM_BEGIN

typedef struct
{
    const char *name;
    const char *type_name;
    uint16_t size;
    uint8_t bit_field;
    uint8_t bit_offset;
    uint8_t bit_size;
} vm_value_meta_t;

vm_status_t vm_value_format(const vm_value_meta_t *meta,
                            const uint8_t *data,
                            size_t data_size,
                            char *out,
                            size_t out_size);

vm_status_t vm_value_to_double(const vm_value_meta_t *meta,
                               const uint8_t *data,
                               size_t data_size,
                               double *out);

vm_status_t vm_value_encode(const vm_value_meta_t *meta,
                            const uint8_t *current_data,
                            size_t current_size,
                            const char *text,
                            uint8_t *out,
                            size_t out_size,
                            size_t *written,
                            char *error,
                            size_t error_size);

VM_END

#endif
