/*
 * 文件说明：CAN/CANFD 传输适配预留实现，后续用于接入厂商驱动。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_can_transport.h"
#include <stdlib.h>
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_can_transport
{
    /* 变量说明：base，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_t base;
    /* 变量说明：config，设备配置缓存。 */
    vm_can_config_t config;
    /* 变量说明：rx，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_transport_rx_fn rx;
    /* 变量说明：rx_context，保存当前对象运行所需的状态、参数或缓存数据。 */
    void *rx_context;
};
/**
 * 函数说明：can_open，打开底层资源。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t can_open(vm_transport_t *b)
{
    (void)b;
    return VM_UNSUPPORTED;
}
/**
 * 函数说明：can_close，关闭底层资源。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t can_close(vm_transport_t *b)
{
    if (b)
    {
        b->opened = 0;
    }
    return VM_OK;
}
/**
 * 函数说明：can_send，执行本模块对应功能逻辑。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。；d：函数输入参数，参与本函数的计算、查找或状态更新。；n：函数输入参数，参与本函数的计算、查找或状态更新。；c：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t
can_send(vm_transport_t *b, const uint8_t *d, size_t n, uint32_t c)
{
    (void)b;
    (void)d;
    (void)n;
    (void)c;
    return VM_UNSUPPORTED;
}
/**
 * 函数说明：can_rx，执行本模块对应功能逻辑。
 * 输入：b：函数输入参数，参与本函数的计算、查找或状态更新。；f：函数输入参数，参与本函数的计算、查找或状态更新。；c：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
static vm_status_t can_rx(vm_transport_t *b, vm_transport_rx_fn f, void *c)
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct vm_can_transport *t = (struct vm_can_transport *)b;
    if (!t)
    {
        /* 变量说明：VM_INVALID，保存当前对象运行所需的状态、参数或缓存数据。 */
        return VM_INVALID;
    }
    t->rx = f;
    t->rx_context = c;
    return VM_OK;
}
static const vm_transport_ops_t ops = {can_open, can_close, can_send, can_rx};
/**
 * 函数说明：vm_can_transport_create，创建并初始化对象。
 * 输入：c：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_can_transport_create(const vm_can_config_t *c,
                                    vm_can_transport_t **out)
{
    vm_can_transport_t *t;
    vm_status_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    s = vm_can_config_validate(c);
    if (s != VM_OK)
    {
        return s;
    }
    *out = NULL;
    t = calloc(1, sizeof(*t));
    if (!t)
    {
        return VM_NOMEM;
    }
    t->config = *c;
    t->base.ops = &ops;
    t->base.context = t;
    *out = t;
    return VM_OK;
}
/**
 * 函数说明：vm_can_transport_destroy，销毁对象并释放相关资源。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_can_transport_destroy(vm_can_transport_t *t)
{
    if (!t)
    {
        return VM_OK;
    }
    can_close(&t->base);
    free(t);
    return VM_OK;
}
/**
 * 函数说明：vm_can_transport_base，执行本模块对应功能逻辑。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
vm_transport_t *vm_can_transport_base(vm_can_transport_t *t)
{
    return t ? t ? &t->base : NULL : NULL;
}
/**
 * 函数说明：vm_can_transport_receive_once，执行本模块对应功能逻辑。
 * 输入：t：函数输入参数，参与本函数的计算、查找或状态更新。；wait_ms：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_can_transport_receive_once(vm_can_transport_t *t,
                                          uint32_t wait_ms)
{
    (void)wait_ms;
    if (!t)
    {
        return VM_INVALID;
    }
    return VM_UNSUPPORTED;
}
