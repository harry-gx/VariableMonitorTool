/*
 * 文件说明：ELF/AXF 解析模块内部接口声明，供模块内部源文件使用。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_REGISTRY_H
#define VM_REGISTRY_H
#include "vm_status.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_registry vm_registry_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：id，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *id;
    /* 变量说明：name，变量名、页面名或节点名。 */
    const char *name;
    /* 变量说明：capabilities，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t capabilities;
} vm_adapter_info_t;
/**`n * 说明：注册表会拷贝适配器元数据，只借用 ops 指针；已 acquire 的 ops 必须 release 后才能注销或销毁。`n * 函数说明：vm_registry_create，创建并初始化对象。`n * 输入：out：输出对象或结果指针，函数成功时写入有效值。`n * 输出：通过返回值、对象成员或输出参数反馈处理结果。`n * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足或底层失败。`n */
vm_status_t vm_registry_create(vm_registry_t **out);
/**
 * 函数说明：vm_registry_destroy，销毁对象并释放相关资源。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_destroy(vm_registry_t *r);
/**
 * 函数说明：vm_registry_add，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；info：函数输入参数，参与本函数的计算、查找或状态更新。；ops：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_add(vm_registry_t *r,
                            const vm_adapter_info_t *info,
                            const void *ops);
/**
 * 函数说明：vm_registry_remove，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_remove(vm_registry_t *r, const char *id);
/**
 * 函数说明：vm_registry_count，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_registry_count(const vm_registry_t *r);
/**
 * 函数说明：vm_registry_at，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_registry_at(const vm_registry_t *r, size_t index, vm_adapter_info_t *out);
/**
 * 函数说明：vm_registry_acquire，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。；ops：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_registry_acquire(vm_registry_t *r, const char *id, const void **ops);
/**
 * 函数说明：vm_registry_release，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_release(vm_registry_t *r, const char *id);
VM_END
#endif
