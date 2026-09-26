#include "vm_value_codec.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    VM_VALUE_UNSIGNED = 0,
    VM_VALUE_SIGNED,
    VM_VALUE_FLOAT32,
    VM_VALUE_FLOAT64,
    VM_VALUE_BYTES
} vm_value_kind_t;

static void
vm_value_set_error(char *error, size_t error_size, const char *message)
{
    if ((error != NULL) && (error_size > 0u))
    {
        if (message == NULL)
        {
            message = "未知错误";
        }

        (void)snprintf(error, error_size, "%s", message);
        error[error_size - 1u] = '\0';
    }
}

static void vm_value_copy_lower(char *out, size_t out_size, const char *input)
{
    size_t index;

    if ((out == NULL) || (out_size == 0u))
    {
        return;
    }

    if (input == NULL)
    {
        out[0] = '\0';
        return;
    }

    while (isspace((unsigned char)*input) != 0)
    {
        ++input;
    }

    for (index = 0u; (index + 1u) < out_size; ++index)
    {
        if (input[index] == '\0')
        {
            break;
        }

        out[index] = (char)tolower((unsigned char)input[index]);
    }

    out[index] = '\0';
    while ((index > 0u) && (isspace((unsigned char)out[index - 1u]) != 0))
    {
        out[index - 1u] = '\0';
        --index;
    }
}

static void vm_value_remove_token(char *text, const char *token)
{
    char *found;
    size_t token_len;
    size_t tail_len;

    if ((text != NULL) && (token != NULL))
    {
        token_len = strlen(token);
        if (token_len > 0u)
        {
            found = strstr(text, token);
            while (found != NULL)
            {
                tail_len = strlen(found + token_len);
                (void)memmove(found, found + token_len, tail_len + 1u);
                found = strstr(text, token);
            }
        }
    }
}

static uint8_t vm_value_text_contains(const char *text, const char *needle)
{
    uint8_t result;

    result = 0u;
    if ((text != NULL) && (needle != NULL))
    {
        if (strstr(text, needle) != NULL)
        {
            result = 1u;
        }
    }

    return result;
}

static uint8_t vm_value_text_starts_with(const char *text, const char *prefix)
{
    uint8_t result;

    result = 0u;
    if ((text != NULL) && (prefix != NULL))
    {
        if (strncmp(text, prefix, strlen(prefix)) == 0)
        {
            result = 1u;
        }
    }

    return result;
}

static uint8_t vm_value_text_equals(const char *left, const char *right)
{
    uint8_t result;

    result = 0u;
    if ((left != NULL) && (right != NULL))
    {
        if (strcmp(left, right) == 0)
        {
            result = 1u;
        }
    }

    return result;
}

static vm_value_kind_t vm_value_infer_kind(const vm_value_meta_t *meta)
{
    char name[128];
    char type[128];
    vm_value_kind_t kind;

    kind = VM_VALUE_BYTES;
    if (meta != NULL)
    {
        vm_value_copy_lower(name, sizeof(name), meta->name);
        vm_value_copy_lower(type, sizeof(type), meta->type_name);
        vm_value_remove_token(type, "volatile ");
        vm_value_remove_token(type, "const ");

        if (type[0] != '\0')
        {
            if ((meta->size == 4u) &&
                ((vm_value_text_equals(type, "float") != 0u) ||
                 (vm_value_text_contains(type, " float") != 0u)))
            {
                kind = VM_VALUE_FLOAT32;
            }
            else if ((meta->size == 8u) &&
                     ((vm_value_text_equals(type, "double") != 0u) ||
                      (vm_value_text_contains(type, " double") != 0u)))
            {
                kind = VM_VALUE_FLOAT64;
            }
            else if ((meta->size >= 1u) && (meta->size <= 8u))
            {
                if ((vm_value_text_starts_with(type, "enum") != 0u) ||
                    (vm_value_text_contains(type, " enum") != 0u))
                {
                    kind = VM_VALUE_SIGNED;
                }
                else if ((vm_value_text_contains(type, "unsigned") != 0u) ||
                         (vm_value_text_contains(type, "__uint") != 0u) ||
                         (vm_value_text_starts_with(type, "uint") != 0u) ||
                         (vm_value_text_equals(type, "bool") != 0u) ||
                         (vm_value_text_equals(type, "uchar") != 0u))
                {
                    kind = VM_VALUE_UNSIGNED;
                }
                else if ((vm_value_text_contains(type, "signed") != 0u) ||
                         (vm_value_text_contains(type, "__int") != 0u) ||
                         (vm_value_text_contains(type, "int") != 0u) ||
                         (vm_value_text_contains(type, "short") != 0u) ||
                         (vm_value_text_contains(type, "long") != 0u) ||
                         (vm_value_text_equals(type, "char") != 0u) ||
                         (vm_value_text_starts_with(type, "int") != 0u))
                {
                    kind = VM_VALUE_SIGNED;
                }
                else
                {
                    kind = VM_VALUE_UNSIGNED;
                }
            }
        }

        if (kind == VM_VALUE_BYTES)
        {
            if ((meta->size == 4u) &&
                ((vm_value_text_equals(name, "cal_speed_setpoint") != 0u) ||
                 (vm_value_text_equals(name, "cal_current_setpoint") != 0u) ||
                 (vm_value_text_equals(name, "cal_speed_kp") != 0u) ||
                 (vm_value_text_equals(name, "cal_speed_ki") != 0u) ||
                 (vm_value_text_equals(name, "cal_speed_kd") != 0u) ||
                 (vm_value_text_equals(name, "cal_current_kp") != 0u) ||
                 (vm_value_text_equals(name, "cal_current_ki") != 0u) ||
                 (vm_value_text_equals(name, "cal_current_kd") != 0u) ||
                 (vm_value_text_equals(name, "mon_bus_voltage") != 0u) ||
                 (vm_value_text_equals(name, "mon_board_temp") != 0u) ||
                 (vm_value_text_equals(name, "mon_current_bus") != 0u) ||
                 (vm_value_text_equals(name, "mon_speed_actual") != 0u) ||
                 (vm_value_text_equals(name, "mon_current_actual") != 0u)))
            {
                kind = VM_VALUE_FLOAT32;
            }
            else if (((meta->size == 4u) &&
                      (vm_value_text_equals(name, "mon_motor_speed") != 0u)) ||
                     ((meta->size == 2u) &&
                      ((vm_value_text_equals(name, "mon_current_u") != 0u) ||
                       (vm_value_text_equals(name, "mon_current_v") != 0u) ||
                       (vm_value_text_equals(name, "mon_current_w") != 0u))))
            {
                kind = VM_VALUE_SIGNED;
            }
            else if ((meta->size >= 1u) && (meta->size <= 8u))
            {
                kind = VM_VALUE_UNSIGNED;
            }
            else
            {
                kind = VM_VALUE_BYTES;
            }
        }
    }

    return kind;
}

static uint64_t
vm_value_read_unsigned_le(const uint8_t *data, size_t data_size, uint16_t size)
{
    uint64_t value;
    size_t count;
    size_t index;

    value = 0u;
    count = size;
    if (count > 8u)
    {
        count = 8u;
    }
    if (count > data_size)
    {
        count = data_size;
    }

    if (data != NULL)
    {
        for (index = 0u; index < count; ++index)
        {
            value |= ((uint64_t)data[index]) << (8u * index);
        }
    }

    return value;
}

static int64_t
vm_value_read_signed_le(const uint8_t *data, size_t data_size, uint16_t size)
{
    uint64_t raw;
    uint64_t sign_bit;
    uint64_t mask;
    int64_t value;

    raw = vm_value_read_unsigned_le(data, data_size, size);
    if ((size == 0u) || (size >= 8u))
    {
        value = (int64_t)raw;
    }
    else
    {
        sign_bit = UINT64_C(1) << (((uint64_t)size * 8u) - 1u);
        mask = (UINT64_C(1) << ((uint64_t)size * 8u)) - 1u;
        if ((raw & sign_bit) == 0u)
        {
            value = (int64_t)raw;
        }
        else
        {
            value = -(int64_t)(((~raw) + 1u) & mask);
        }
    }

    return value;
}

static float vm_value_read_float32_le(const uint8_t *data, size_t data_size)
{
    uint32_t raw;
    float value;

    raw = (uint32_t)vm_value_read_unsigned_le(data, data_size, 4u);
    value = 0.0F;
    (void)memcpy(&value, &raw, sizeof(value));
    return value;
}

static double vm_value_read_float64_le(const uint8_t *data, size_t data_size)
{
    uint64_t raw;
    double value;

    raw = vm_value_read_unsigned_le(data, data_size, 8u);
    value = 0.0;
    (void)memcpy(&value, &raw, sizeof(value));
    return value;
}

static void vm_value_trim_float_text(char *text)
{
    size_t length;

    if (text != NULL)
    {
        if (strchr(text, '.') != NULL)
        {
            length = strlen(text);
            while ((length > 0u) && (text[length - 1u] == '0'))
            {
                text[length - 1u] = '\0';
                --length;
            }

            if ((length > 0u) && (text[length - 1u] == '.'))
            {
                text[length - 1u] = '\0';
            }
        }
    }
}

static vm_status_t
vm_value_write_text(char *out, size_t out_size, const char *text)
{
    vm_status_t status;

    status = VM_INVALID;
    if ((out != NULL) && (out_size > 0u) && (text != NULL))
    {
        if (snprintf(out, out_size, "%s", text) < (int)out_size)
        {
            status = VM_OK;
        }
        else
        {
            status = VM_NOMEM;
        }
        out[out_size - 1u] = '\0';
    }

    return status;
}

static vm_status_t
vm_value_u64_to_text(uint64_t value, char *out, size_t out_size)
{
    char buffer[21];
    size_t index;

    index = sizeof(buffer);
    --index;
    buffer[index] = '\0';

    do
    {
        --index;
        buffer[index] = (char)('0' + (char)(value % 10u));
        value /= 10u;
    } while ((value != 0u) && (index > 0u));

    return vm_value_write_text(out, out_size, &buffer[index]);
}

static vm_status_t
vm_value_i64_to_text(int64_t value, char *out, size_t out_size)
{
    char buffer[22];
    uint64_t magnitude;
    size_t index;

    index = sizeof(buffer);
    --index;
    buffer[index] = '\0';

    if (value < 0)
    {
        magnitude = (uint64_t)(-(value + 1)) + UINT64_C(1);
    }
    else
    {
        magnitude = (uint64_t)value;
    }

    do
    {
        --index;
        buffer[index] = (char)('0' + (char)(magnitude % 10u));
        magnitude /= 10u;
    } while ((magnitude != 0u) && (index > 0u));

    if ((value < 0) && (index > 0u))
    {
        --index;
        buffer[index] = '-';
    }

    return vm_value_write_text(out, out_size, &buffer[index]);
}

static uint64_t vm_value_bit_mask(uint8_t bit_size)
{
    uint64_t mask;

    if (bit_size >= 64u)
    {
        mask = UINT64_MAX;
    }
    else
    {
        mask = (UINT64_C(1) << bit_size) - 1u;
    }

    return mask;
}

static uint8_t
vm_value_valid_bit_field(uint16_t size, uint8_t bit_offset, uint8_t bit_size)
{
    uint32_t total_bits;
    uint8_t result;

    total_bits = (uint32_t)size * 8u;
    result =
        (uint8_t)((size > 0u) && (size <= 8u) && (bit_size > 0u) &&
                  (bit_offset < 64u) &&
                  (((uint32_t)bit_offset + (uint32_t)bit_size) <= total_bits));

    return result;
}

static uint64_t vm_value_read_bit_field_unsigned(const uint8_t *data,
                                                 size_t data_size,
                                                 uint16_t size,
                                                 uint8_t bit_offset,
                                                 uint8_t bit_size)
{
    return (vm_value_read_unsigned_le(data, data_size, size) >> bit_offset) &
           vm_value_bit_mask(bit_size);
}

static int64_t vm_value_sign_extend_bit_field(uint64_t raw, uint8_t bit_size)
{
    uint64_t sign_bit;
    uint64_t mask;
    int64_t value;

    if ((bit_size == 0u) || (bit_size >= 64u))
    {
        value = (int64_t)raw;
    }
    else
    {
        sign_bit = UINT64_C(1) << (bit_size - 1u);
        mask = vm_value_bit_mask(bit_size);
        if ((raw & sign_bit) == 0u)
        {
            value = (int64_t)raw;
        }
        else
        {
            value = -(int64_t)(((~raw) + 1u) & mask);
        }
    }

    return value;
}

static vm_status_t vm_value_format_bytes(const uint8_t *data,
                                         size_t data_size,
                                         char *out,
                                         size_t out_size)
{
    size_t index;
    size_t used;
    int written;
    vm_status_t status;

    status = VM_INVALID;
    if ((out != NULL) && (out_size > 0u))
    {
        out[0] = '\0';
        status = VM_OK;
        used = 0u;
        for (index = 0u; index < data_size; ++index)
        {
            written = snprintf(&out[used],
                               out_size - used,
                               (index == 0u) ? "%u" : " %u",
                               (unsigned)data[index]);
            if ((written < 0) || ((size_t)written >= (out_size - used)))
            {
                status = VM_NOMEM;
                break;
            }

            used += (size_t)written;
        }
    }

    return status;
}

static void
vm_value_append_unsigned_le(uint8_t *out, uint64_t value, uint16_t size)
{
    uint16_t index;

    if (out != NULL)
    {
        for (index = 0u; index < size; ++index)
        {
            out[index] = (uint8_t)((value >> (8u * index)) & 0xFFu);
        }
    }
}

static const char *
vm_value_trim_input(const char *text, char *buffer, size_t buffer_size)
{
    size_t index;
    size_t length;
    const char *start;

    if ((text == NULL) || (buffer == NULL) || (buffer_size == 0u))
    {
        return NULL;
    }

    start = text;
    while (isspace((unsigned char)*start) != 0)
    {
        ++start;
    }

    for (index = 0u; (index + 1u) < buffer_size; ++index)
    {
        if (start[index] == '\0')
        {
            break;
        }

        buffer[index] = start[index];
    }
    buffer[index] = '\0';

    length = strlen(buffer);
    while ((length > 0u) && (isspace((unsigned char)buffer[length - 1u]) != 0))
    {
        buffer[length - 1u] = '\0';
        --length;
    }

    return buffer;
}

vm_status_t vm_value_format(const vm_value_meta_t *meta,
                            const uint8_t *data,
                            size_t data_size,
                            char *out,
                            size_t out_size)
{
    uint64_t raw;
    int64_t signed_value;
    vm_status_t status;
    int written;

    status = VM_INVALID;
    if ((meta != NULL) && (out != NULL) && (out_size > 0u))
    {
        if ((data == NULL) || (data_size == 0u))
        {
            status = vm_value_write_text(out, out_size, "-");
        }
        else if (meta->bit_field != 0u)
        {
            if ((vm_value_valid_bit_field(
                     meta->size, meta->bit_offset, meta->bit_size) == 0u) ||
                (data_size < meta->size))
            {
                status = vm_value_write_text(out, out_size, "-");
            }
            else
            {
                raw = vm_value_read_bit_field_unsigned(data,
                                                       data_size,
                                                       meta->size,
                                                       meta->bit_offset,
                                                       meta->bit_size);
                if (vm_value_infer_kind(meta) == VM_VALUE_SIGNED)
                {
                    signed_value =
                        vm_value_sign_extend_bit_field(raw, meta->bit_size);
                    status = vm_value_i64_to_text(signed_value, out, out_size);
                }
                else
                {
                    status = vm_value_u64_to_text(raw, out, out_size);
                }
            }
        }
        else
        {
            switch (vm_value_infer_kind(meta))
            {
                case VM_VALUE_FLOAT32:
                    if (data_size < 4u)
                    {
                        status = vm_value_write_text(out, out_size, "-");
                    }
                    else
                    {
                        written = snprintf(
                            out,
                            out_size,
                            "%.3f",
                            (double)vm_value_read_float32_le(data, data_size));
                        status = (written >= 0 && (size_t)written < out_size)
                                     ? VM_OK
                                     : VM_NOMEM;
                        if (status == VM_OK)
                        {
                            vm_value_trim_float_text(out);
                        }
                    }
                    break;

                case VM_VALUE_FLOAT64:
                    if (data_size < 8u)
                    {
                        status = vm_value_write_text(out, out_size, "-");
                    }
                    else
                    {
                        written =
                            snprintf(out,
                                     out_size,
                                     "%.6f",
                                     vm_value_read_float64_le(data, data_size));
                        status = (written >= 0 && (size_t)written < out_size)
                                     ? VM_OK
                                     : VM_NOMEM;
                        if (status == VM_OK)
                        {
                            vm_value_trim_float_text(out);
                        }
                    }
                    break;

                case VM_VALUE_SIGNED:
                    signed_value =
                        vm_value_read_signed_le(data, data_size, meta->size);
                    status = vm_value_i64_to_text(signed_value, out, out_size);
                    break;

                case VM_VALUE_UNSIGNED:
                    raw =
                        vm_value_read_unsigned_le(data, data_size, meta->size);
                    status = vm_value_u64_to_text(raw, out, out_size);
                    break;

                case VM_VALUE_BYTES:
                default:
                    status =
                        vm_value_format_bytes(data, data_size, out, out_size);
                    break;
            }
        }

        out[out_size - 1u] = '\0';
    }

    return status;
}

vm_status_t vm_value_to_double(const vm_value_meta_t *meta,
                               const uint8_t *data,
                               size_t data_size,
                               double *out)
{
    uint64_t raw;
    vm_status_t status;

    status = VM_INVALID;
    if ((meta != NULL) && (out != NULL))
    {
        *out = 0.0;
        if ((data == NULL) || (data_size == 0u))
        {
            status = VM_OK;
        }
        else if (meta->bit_field != 0u)
        {
            if ((vm_value_valid_bit_field(
                     meta->size, meta->bit_offset, meta->bit_size) != 0u) &&
                (data_size >= meta->size))
            {
                raw = vm_value_read_bit_field_unsigned(data,
                                                       data_size,
                                                       meta->size,
                                                       meta->bit_offset,
                                                       meta->bit_size);
                if (vm_value_infer_kind(meta) == VM_VALUE_SIGNED)
                {
                    *out = (double)vm_value_sign_extend_bit_field(
                        raw, meta->bit_size);
                }
                else
                {
                    *out = (double)raw;
                }
            }
            status = VM_OK;
        }
        else
        {
            switch (vm_value_infer_kind(meta))
            {
                case VM_VALUE_FLOAT32:
                    *out =
                        (data_size >= 4u)
                            ? (double)vm_value_read_float32_le(data, data_size)
                            : 0.0;
                    break;
                case VM_VALUE_FLOAT64:
                    *out = (data_size >= 8u)
                               ? vm_value_read_float64_le(data, data_size)
                               : 0.0;
                    break;
                case VM_VALUE_SIGNED:
                    *out = (double)vm_value_read_signed_le(
                        data, data_size, meta->size);
                    break;
                case VM_VALUE_UNSIGNED:
                    *out = (double)vm_value_read_unsigned_le(
                        data, data_size, meta->size);
                    break;
                case VM_VALUE_BYTES:
                default:
                    *out = 0.0;
                    break;
            }
            status = VM_OK;
        }
    }

    return status;
}

static vm_status_t vm_value_encode_bit_field(const vm_value_meta_t *meta,
                                             const uint8_t *current_data,
                                             size_t current_size,
                                             const char *input,
                                             uint8_t *out,
                                             size_t out_size,
                                             size_t *written,
                                             char *error,
                                             size_t error_size)
{
    char *end_ptr;
    int64_t signed_value;
    uint64_t unsigned_value;
    int64_t min_value;
    int64_t max_value;
    uint64_t encoded;
    uint64_t mask;
    uint64_t raw;
    vm_status_t status;

    status = VM_INVALID;
    if ((meta == NULL) || (current_data == NULL) || (out == NULL) ||
        (written == NULL))
    {
        vm_value_set_error(error, error_size, "参数错误");
    }
    else if ((vm_value_valid_bit_field(
                  meta->size, meta->bit_offset, meta->bit_size) == 0u) ||
             (current_size < meta->size) || (out_size < meta->size))
    {
        vm_value_set_error(
            error, error_size, "位字段缺少当前原始值，请先读取一次当前值");
    }
    else
    {
        encoded = 0u;
        errno = 0;
        if (vm_value_infer_kind(meta) == VM_VALUE_SIGNED)
        {
            signed_value = strtoll(input, &end_ptr, 0);
            min_value = (meta->bit_size >= 64u)
                            ? INT64_MIN
                            : -(INT64_C(1) << (meta->bit_size - 1u));
            max_value = (meta->bit_size >= 64u)
                            ? INT64_MAX
                            : ((INT64_C(1) << (meta->bit_size - 1u)) - 1);
            if ((errno != 0) || (end_ptr == input) || (*end_ptr != '\0') ||
                (signed_value < min_value) || (signed_value > max_value))
            {
                vm_value_set_error(
                    error, error_size, "目标值超出位字段有符号范围");
            }
            else
            {
                encoded =
                    (uint64_t)signed_value & vm_value_bit_mask(meta->bit_size);
                status = VM_OK;
            }
        }
        else
        {
            unsigned_value = strtoull(input, &end_ptr, 0);
            if ((errno != 0) || (end_ptr == input) || (*end_ptr != '\0') ||
                (unsigned_value > vm_value_bit_mask(meta->bit_size)))
            {
                vm_value_set_error(
                    error, error_size, "目标值超出位字段无符号范围");
            }
            else
            {
                encoded = unsigned_value;
                status = VM_OK;
            }
        }

        if (status == VM_OK)
        {
            mask = vm_value_bit_mask(meta->bit_size) << meta->bit_offset;
            raw = vm_value_read_unsigned_le(
                current_data, current_size, meta->size);
            raw = (raw & ~mask) | ((encoded << meta->bit_offset) & mask);
            vm_value_append_unsigned_le(out, raw, meta->size);
            *written = meta->size;
        }
    }

    return status;
}

vm_status_t vm_value_encode(const vm_value_meta_t *meta,
                            const uint8_t *current_data,
                            size_t current_size,
                            const char *text,
                            uint8_t *out,
                            size_t out_size,
                            size_t *written,
                            char *error,
                            size_t error_size)
{
    char input[128];
    const char *trimmed;
    char *end_ptr;
    float float_value;
    double double_value;
    uint32_t raw32;
    uint64_t raw64;
    int64_t signed_value;
    uint64_t unsigned_value;
    int64_t min_value;
    int64_t max_value;
    uint64_t max_unsigned;
    vm_status_t status;

    status = VM_INVALID;
    if (written != NULL)
    {
        *written = 0u;
    }

    trimmed = vm_value_trim_input(text, input, sizeof(input));
    if ((meta == NULL) || (out == NULL) || (written == NULL) ||
        (trimmed == NULL))
    {
        vm_value_set_error(error, error_size, "参数错误");
    }
    else if (input[0] == '\0')
    {
        vm_value_set_error(error, error_size, "目标值不能为空");
    }
    else if (meta->bit_field != 0u)
    {
        status = vm_value_encode_bit_field(meta,
                                           current_data,
                                           current_size,
                                           input,
                                           out,
                                           out_size,
                                           written,
                                           error,
                                           error_size);
    }
    else
    {
        switch (vm_value_infer_kind(meta))
        {
            case VM_VALUE_FLOAT32:
                errno = 0;
                float_value = strtof(input, &end_ptr);
                if ((errno != 0) || (end_ptr == input) || (*end_ptr != '\0') ||
                    (meta->size != 4u) || (out_size < 4u) ||
                    (isfinite((double)float_value) == 0))
                {
                    vm_value_set_error(
                        error, error_size, "目标值必须是十进制浮点数");
                }
                else
                {
                    (void)memcpy(&raw32, &float_value, sizeof(raw32));
                    vm_value_append_unsigned_le(out, raw32, 4u);
                    *written = 4u;
                    status = VM_OK;
                }
                break;

            case VM_VALUE_FLOAT64:
                errno = 0;
                double_value = strtod(input, &end_ptr);
                if ((errno != 0) || (end_ptr == input) || (*end_ptr != '\0') ||
                    (meta->size != 8u) || (out_size < 8u) ||
                    (isfinite(double_value) == 0))
                {
                    vm_value_set_error(
                        error, error_size, "目标值必须是十进制浮点数");
                }
                else
                {
                    (void)memcpy(&raw64, &double_value, sizeof(raw64));
                    vm_value_append_unsigned_le(out, raw64, 8u);
                    *written = 8u;
                    status = VM_OK;
                }
                break;

            case VM_VALUE_SIGNED:
                errno = 0;
                signed_value = strtoll(input, &end_ptr, 0);
                min_value = (meta->size >= 8u)
                                ? INT64_MIN
                                : -(INT64_C(1) << ((meta->size * 8u) - 1u));
                max_value =
                    (meta->size >= 8u)
                        ? INT64_MAX
                        : ((INT64_C(1) << ((meta->size * 8u) - 1u)) - 1);
                if ((errno != 0) || (end_ptr == input) || (*end_ptr != '\0') ||
                    (signed_value < min_value) || (signed_value > max_value) ||
                    (out_size < meta->size))
                {
                    vm_value_set_error(
                        error, error_size, "目标值超出有符号整数范围");
                }
                else
                {
                    vm_value_append_unsigned_le(
                        out, (uint64_t)signed_value, meta->size);
                    *written = meta->size;
                    status = VM_OK;
                }
                break;

            case VM_VALUE_UNSIGNED:
                errno = 0;
                unsigned_value = strtoull(input, &end_ptr, 0);
                max_unsigned = (meta->size >= 8u)
                                   ? UINT64_MAX
                                   : ((UINT64_C(1) << (meta->size * 8u)) - 1u);
                if ((errno != 0) || (end_ptr == input) || (*end_ptr != '\0') ||
                    (unsigned_value > max_unsigned) || (out_size < meta->size))
                {
                    vm_value_set_error(
                        error, error_size, "目标值超出无符号整数范围");
                }
                else
                {
                    vm_value_append_unsigned_le(
                        out, unsigned_value, meta->size);
                    *written = meta->size;
                    status = VM_OK;
                }
                break;

            case VM_VALUE_BYTES:
            default:
                vm_value_set_error(
                    error, error_size, "暂不支持该长度变量的十进制写入");
                break;
        }
    }

    return status;
}
