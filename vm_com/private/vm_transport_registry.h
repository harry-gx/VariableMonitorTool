/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_TRANSPORT_REGISTRY_H
#define VM_TRANSPORT_REGISTRY_H
#include "vm_transport.h"
#include "vm_list.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_transport_registry vm_transport_registry_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_transport_node
{
    /* 变量说明：id，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *id;
    /* 变量说明：display_name，保存当前对象运行所需的状态、参数或缓存数据。 */
    const char *display_name;
    /* 变量说明：ops，操作函数表指针。 */
    const vm_transport_ops_t *ops;
    /* 变量说明：context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *context;
    /* 变量说明：link，链表节点，用于挂接到注册表或路由表。 */
    list_head_t link;
    /* 变量说明：owner，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_registry_t *owner;
    /* 变量说明：references，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t references;
} vm_transport_node_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_transport_registry
{
    /* 变量说明：head，链表头节点。 */
    list_head_t head;
};
/**
 * 函数说明：vm_transport_registry_init，初始化上下文和默认状态。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_transport_registry_init(vm_transport_registry_t *registry);
/**
 * 函数说明：vm_transport_registry_register，注册节点到内部表。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。；node：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_register(vm_transport_registry_t *registry,
                                           vm_transport_node_t *node);
/**
 * 函数说明：vm_transport_registry_unregister，注册节点到内部表。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。；node：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_unregister(vm_transport_registry_t *registry,
                                             vm_transport_node_t *node);
/**
 * 函数说明：vm_transport_registry_find，查找匹配对象。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回对象指针或缓冲区指针，返回 NULL 表示未找到或失败。
 */
vm_transport_node_t *
vm_transport_registry_find(vm_transport_registry_t *registry, const char *id);
/**
 * 函数说明：vm_transport_registry_count，执行本模块对应功能逻辑。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_transport_registry_count(const vm_transport_registry_t *registry);
/**
 * 函数说明：vm_transport_registry_at，执行本模块对应功能逻辑。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_at(const vm_transport_registry_t *registry,
                                     size_t index,
                                     vm_transport_node_t **out);
/**
 * 函数说明：vm_transport_registry_acquire，执行本模块对应功能逻辑。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_acquire(vm_transport_registry_t *registry,
                                          const char *id,
                                          vm_transport_node_t **out);
/**
 * 函数说明：vm_transport_registry_release，执行本模块对应功能逻辑。
 * 输入：registry：函数输入参数，参与本函数的计算、查找或状态更新。；node：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_release(vm_transport_registry_t *registry,
                                          vm_transport_node_t *node);
VM_END
#endif
