/*
 * 文件说明：通信模块内部路由层实现，负责在传输层和协议层之间转发数据帧。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_router.h"
#include "vm_list.h"
#include <stdlib.h>
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：link，链表节点，用于挂接到注册表或路由表。 */
    list_head_t link;
    /* 变量说明：route，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_route_t route;
} binding;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_router
{
    /* 变量说明：head，链表头节点。 */
    list_head_t head;
    /* 变量说明：dispatching，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t dispatching;
};
/**
 * 函数说明：vm_router_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_create(vm_router_t **out)
{
    vm_router_t *r;
    if (!out)
    {
        return VM_INVALID;
    }
    *out = NULL;
    r = calloc(1, sizeof(*r));
    if (!r)
    {
        return VM_NOMEM;
    }
    INIT_LIST_HEAD(&r->head);
    *out = r;
    return VM_OK;
}
/**
 * 函数说明：vm_router_bind，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；v：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_bind(vm_router_t *r, const vm_route_t *v)
{
    list_head_t *p;
    binding *b;
    if (!r || !v || !v->protocol_receive || !v->transport_send)
    {
        return VM_INVALID;
    }
    if (r->dispatching)
    {
        return VM_BUSY;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        const vm_route_t *x = &list_entry(p, binding, link)->route;
        if (x->route_id == v->route_id)
        {
            return VM_DUPLICATE;
        }
        if (x->connection == v->connection && x->channel == v->channel &&
            x->rx_address == v->rx_address)
        {
            return VM_AMBIGUOUS;
        }
    }
    b = malloc(sizeof(*b));
    if (!b)
    {
        return VM_NOMEM;
    }
    b->route = *v;
    INIT_LIST_HEAD(&b->link);
    LIST_ADD_TAIL(&b->link, &r->head);
    return VM_OK;
}
/**
 * 函数说明：vm_router_unbind，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_unbind(vm_router_t *r, uint64_t id)
{
    list_head_t *p;
    if (!r)
    {
        return VM_INVALID;
    }
    if (r->dispatching)
    {
        return VM_BUSY;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        binding *b = list_entry(p, binding, link);
        if (b->route.route_id == id)
        {
            LIST_DEL(p);
            free(b);
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_router_destroy，销毁对象并释放相关资源。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_destroy(vm_router_t *r)
{
    if (!r)
    {
        return VM_OK;
    }
    if (r->dispatching)
    {
        return VM_BUSY;
    }
    while (!LIST_EMPTY(&r->head))
    {
        binding *b = list_entry(r->head.next, binding, link);
        LIST_DEL(&b->link);
        free(b);
    }
    free(r);
    return VM_OK;
}
/**
 * 函数说明：vm_router_receive，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；f：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_router_receive(vm_router_t *r, const vm_frame_t *f)
{
    list_head_t *p;
    if (!r || !f || (!f->data && f->size))
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        vm_route_t *v = &list_entry(p, binding, link)->route;
        if (v->connection == f->connection && v->channel == f->channel &&
            v->rx_address == f->address)
        {
            vm_status_t s;
            r->dispatching++;
            s = v->protocol_receive(v->protocol_context, f);
            r->dispatching--;
            return s;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_router_send，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。；data：输入或输出的原始字节缓冲区。；size：数据长度或缓冲区容量。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_router_send(vm_router_t *r, uint64_t id, const uint8_t *data, size_t size)
{
    list_head_t *p;
    if (!r || (!data && size))
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        vm_route_t *v = &list_entry(p, binding, link)->route;
        if (v->route_id == id)
        {
            vm_frame_t f;
            vm_status_t s;
            f.connection = v->connection;
            f.channel = v->channel;
            f.address = v->tx_address;
            f.data = data;
            f.size = size;
            r->dispatching++;
            s = v->transport_send(v->transport_context, &f);
            r->dispatching--;
            return s;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_router_route_count，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_router_route_count(const vm_router_t *r)
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
 * 函数说明：vm_router_route_at，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_router_route_at(const vm_router_t *r, size_t index, vm_route_t *out)
{
    list_head_t *p;
    if (!r || !out)
    {
        return VM_INVALID;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (index-- == 0)
        {
            *out = list_entry(p, binding, link)->route;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
