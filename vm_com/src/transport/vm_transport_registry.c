/*
 * 文件说明：传输通道注册表实现，负责传输节点注册、查找、引用计数和注销。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_transport_registry.h"
#include <string.h>
/**
 * 函数说明：vm_transport_registry_init，初始化上下文和默认状态。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_transport_registry_init(vm_transport_registry_t *r)
{
    if (r)
    {
        INIT_LIST_HEAD(&r->head);
    }
}
/**
 * 函数说明：vm_transport_registry_find，查找匹配对象。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
vm_transport_node_t *vm_transport_registry_find(vm_transport_registry_t *r,
                                                const char *id)
{
    list_head_t *p;
    if (!r || !id)
    {
        return NULL;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        vm_transport_node_t *n = list_entry(p, vm_transport_node_t, link);
        if (!strcmp(n->id, id))
        {
            return n;
        }
    }
    return NULL;
}
/**
 * 函数说明：vm_transport_registry_register，注册节点到内部表。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；n：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_register(vm_transport_registry_t *r,
                                           vm_transport_node_t *n)
{
    if (!r || !n || !n->id || !*n->id || !n->display_name ||
        !*n->display_name || !n->ops || !n->ops->open || !n->ops->close ||
        !n->ops->send || !n->ops->set_rx)
    {
        return VM_INVALID;
    }
    if (n->owner)
    {
        return n->owner == r ? VM_DUPLICATE : VM_BUSY;
    }
    if (vm_transport_registry_find(r, n->id))
    {
        return VM_DUPLICATE;
    }
    INIT_LIST_HEAD(&n->link);
    LIST_ADD_TAIL(&n->link, &r->head);
    n->owner = r;
    n->references = 0;
    return VM_OK;
}
/**
 * 函数说明：vm_transport_registry_unregister，注册节点到内部表。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；n：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_unregister(vm_transport_registry_t *r,
                                             vm_transport_node_t *n)
{
    list_head_t *p;
    if (!r || !n)
    {
        return VM_INVALID;
    }
    if (n->owner != r)
    {
        return VM_NOT_FOUND;
    }
    if (n->references)
    {
        return VM_BUSY;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (list_entry(p, vm_transport_node_t, link) == n)
        {
            LIST_DEL(&n->link);
            INIT_LIST_HEAD(&n->link);
            n->owner = NULL;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_transport_registry_count，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_transport_registry_count(const vm_transport_registry_t *r)
{
    size_t n = 0;
    list_head_t *p;
    if (!r)
    {
        return 0;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        ++n;
    }
    return n;
}
/**
 * 函数说明：vm_transport_registry_at，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；i：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_at(const vm_transport_registry_t *r,
                                     size_t i,
                                     vm_transport_node_t **out)
{
    list_head_t *p;
    if (!r || !out)
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (i-- == 0)
        {
            *out = list_entry(p, vm_transport_node_t, link);
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_transport_registry_acquire，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_acquire(vm_transport_registry_t *r,
                                          const char *id,
                                          vm_transport_node_t **out)
{
    vm_transport_node_t *n;
    if (!r || !id || !out)
    {
        return VM_INVALID;
    }
    n = vm_transport_registry_find(r, id);
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (n->references == SIZE_MAX)
    {
        return VM_BUSY;
    }
    ++n->references;
    *out = n;
    return VM_OK;
}
/**
 * 函数说明：vm_transport_registry_release，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；n：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_transport_registry_release(vm_transport_registry_t *r,
                                          vm_transport_node_t *n)
{
    if (!r || !n)
    {
        return VM_INVALID;
    }
    if (n->owner != r)
    {
        return VM_NOT_FOUND;
    }
    if (!n->references)
    {
        return VM_INVALID;
    }
    --n->references;
    return VM_OK;
}
