/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_VALUE_CODEC_H
#define VM_VALUE_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "vm_status.h"

VM_BEGIN

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：name，变量名、页面名或节点名。 */
    const char *name;
    /* 变量说明：type_name，变量类型名称。 */
    const char *type_name;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint16_t size;
    /* 变量说明：bit_field，是否为位字段变量。 */
    uint8_t bit_field;
    /* 变量说明：bit_offset，位字段起始 bit 偏移。 */
    uint8_t bit_offset;
    /* 变量说明：bit_size，位字段 bit 宽度。 */
    uint8_t bit_size;
} vm_value_meta_t;

/**
 * 函数说明：vm_value_format，执行本模块对应功能逻辑。
 * 输入：meta：函数输入参数，参与本函数的计算、查找或状态更新。；data：输入或输出的原始字节缓冲区。；data_size：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。；out_size：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_value_format(const vm_value_meta_t *meta,
                            const uint8_t *data,
                            size_t data_size,
                            char *out,
                            size_t out_size);

/**
 * 函数说明：vm_value_to_double，执行本模块对应功能逻辑。
 * 输入：meta：函数输入参数，参与本函数的计算、查找或状态更新。；data：输入或输出的原始字节缓冲区。；data_size：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_value_to_double(const vm_value_meta_t *meta,
                               const uint8_t *data,
                               size_t data_size,
                               double *out);

/**
 * 函数说明：vm_value_encode，编码数据到传输格式。
 * 输入：meta：函数输入参数，参与本函数的计算、查找或状态更新。；current_data：函数输入参数，参与本函数的计算、查找或状态更新。；current_size：函数输入参数，参与本函数的计算、查找或状态更新。；text：文本内容或输入字符串。；out：输出对象或结果指针，函数成功时写入有效值。；out_size：函数输入参数，参与本函数的计算、查找或状态更新。；written：函数输入参数，参与本函数的计算、查找或状态更新。；error：错误信息输出缓冲区。；error_size：错误信息缓冲区长度。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
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
