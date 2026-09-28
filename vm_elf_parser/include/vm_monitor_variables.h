/*
 * 文件说明：ELF/AXF 变量解析模块对外接口，输出可监控和可标定变量列表。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_MONITOR_VARIABLES_H
#define VM_MONITOR_VARIABLES_H

#include <stddef.h>
#include <stdint.h>

#include "vm_status.h"

VM_BEGIN

/* 常量说明：VM_MONITOR_NAME_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_MONITOR_NAME_MAX (256u)
/* 常量说明：VM_MONITOR_TYPE_NAME_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_MONITOR_TYPE_NAME_MAX (96u)
/* 常量说明：VM_MONITOR_ERROR_MAX 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define VM_MONITOR_ERROR_MAX (256u)

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_monitor_variable_list vm_monitor_variable_list_t;

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：name，变量名、页面名或节点名。 */
    char name[VM_MONITOR_NAME_MAX];
    /* 变量说明：type_name，变量类型名称。 */
    char type_name[VM_MONITOR_TYPE_NAME_MAX];
    /* 变量说明：address，MCU 目标内存地址或协议地址。 */
    uint64_t address;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint64_t size;
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
} vm_monitor_variable_t;

/**
 * 函数说明：vm_monitor_variables_load，加载外部文件或配置。
 * 输入：path：待加载的文件路径。；out：输出对象或结果指针，函数成功时写入有效值。；error：错误信息输出缓冲区。；error_size：错误信息缓冲区长度。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_monitor_variables_load(const char *path,
                                      vm_monitor_variable_list_t **out,
                                      char *error,
                                      size_t error_size);

/**
 * 函数说明：vm_monitor_variable_count，执行本模块对应功能逻辑。
 * 输入：list：变量列表或节点列表对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_monitor_variable_count(const vm_monitor_variable_list_t *list);

/**
 * 函数说明：vm_monitor_variable_at，执行本模块对应功能逻辑。
 * 输入：list：变量列表或节点列表对象。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_monitor_variable_at(const vm_monitor_variable_list_t *list,
                                   size_t index,
                                   vm_monitor_variable_t *out);

/**
 * 函数说明：vm_monitor_variable_list_destroy，销毁对象并释放相关资源。
 * 输入：list：变量列表或节点列表对象。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_monitor_variable_list_destroy(vm_monitor_variable_list_t *list);

VM_END

#endif
