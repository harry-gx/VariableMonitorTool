/*
 * 文件说明：回环传输适配实现，用于通信模块内部链路自测。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_loopback_transport.h"
#include <stdlib.h>
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_loopback_transport
{
    /* 变量说明：base，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_t base;
    /* 变量说明：rx，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_rx_fn rx;
    /* 变量说明：context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *context;
};
/**
 * 函数说明：open_loop，打开底层资源。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t open_loop(vm_transport_t *b)
{
    if (!b)
    {
        return VM_INVALID;
    }
    b->opened = 1;
    return VM_OK;
}
/**
 * 函数说明：close_loop，关闭底层资源。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t close_loop(vm_transport_t *b)
{
    if (!b)
    {
        return VM_INVALID;
    }
    b->opened = 0;
    return VM_OK;
}
/**
 * 函数说明：send_loop，执行本模块对应功能逻辑。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。；d：函数输入参数，参与本函数的计算、查找或状态更新。；n：函数输入参数，参与本函数的计算、查找或状态更新。；ch：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t
send_loop(vm_transport_t *b, const uint8_t *d, size_t n, uint32_t ch)
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct vm_loopback_transport *t;
    if (!b || (!d && n))
    {
        /* 变量说明：VM_INVALID，保存当前对象运行所需的状态、参数或缓存数据。 */
        return VM_INVALID;
    }
    if (!b->opened)
    {
        return VM_BUSY;
    }
    t = (struct vm_loopback_transport *)b;
    if (!t->rx)
    {
        return VM_NOT_FOUND;
    }
    {
        vm_transport_packet_t p = {d, n, ch, 0};
        return t->rx(t->context, &p);
    }
}
/**
 * 函数说明：set_loop，执行本模块对应功能逻辑。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。；f：函数输入参数，参与本函数的计算、查找或状态更新。；c：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t set_loop(vm_transport_t *b, vm_transport_rx_fn f, void *c)
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct vm_loopback_transport *t;
    if (!b)
    {
        /* 变量说明：VM_INVALID，保存当前对象运行所需的状态、参数或缓存数据。 */
        return VM_INVALID;
    }
    t = (struct vm_loopback_transport *)b;
    t->rx = f;
    t->context = c;
    return VM_OK;
}
static const vm_transport_ops_t ops = {
    open_loop, close_loop, send_loop, set_loop};
/**
 * 函数说明：vm_loopback_transport_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_loopback_transport_create(vm_loopback_transport_t **out)
{
    vm_loopback_transport_t *t;
    if (!out)
    {
        return VM_INVALID;
    }
    *out = NULL;
    t = calloc(1, sizeof(*t));
    if (!t)
    {
        return VM_NOMEM;
    }
    t->base.ops = &ops;
    t->base.context = t;
    *out = t;
    return VM_OK;
}
/**
 * 函数说明：vm_loopback_transport_destroy，销毁对象并释放相关资源。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_loopback_transport_destroy(vm_loopback_transport_t *t)
{
    if (!t)
    {
        return VM_OK;
    }
    free(t);
    return VM_OK;
}
/**
 * 函数说明：vm_loopback_transport_base，执行本模块对应功能逻辑。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
vm_transport_t *vm_loopback_transport_base(vm_loopback_transport_t *t)
{
    return t ? &t->base : NULL;
}
