/*
 * 文件说明：通用注册表实现，负责对象列表的注册、查找和引用计数管理。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_registry.h"
#include "vm_list.h"
#include <stdlib.h>
#include <string.h>
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：link，链表节点，用于挂接到注册表或路由表。 */
    list_head_t link;
    /* 变量说明：info，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_adapter_info_t info;
    /* 变量说明：ops，操作函数表指针。 */
    const void *ops;
    /* 变量说明：refs，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t refs;
} node;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_registry
{
    /* 变量说明：head，链表头节点。 */
    list_head_t head;
    /* 变量说明：count，当前元素数量。 */
    size_t count;
};
/**
 * 函数说明：copy，复制文本或结构化数据。
 * 输入：s：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static char *copy(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p)
    {
        memcpy(p, s, n);
    }
    return p;
}
/**
 * 函数说明：find，查找匹配对象。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static node *find(vm_registry_t *r, const char *id)
{
    list_head_t *p;
    if (!r || !id)
    {
        return NULL;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        node *n = list_entry(p, node, link);
        if (!strcmp(n->info.id, id))
        {
            return n;
        }
    }
    return NULL;
}
/**
 * 函数说明：vm_registry_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_create(vm_registry_t **out)
{
    vm_registry_t *r;
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
 * 函数说明：vm_registry_add，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；i：函数输入参数，参与本函数的计算、查找或状态更新。；ops：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_registry_add(vm_registry_t *r, const vm_adapter_info_t *i, const void *ops)
{
    node *n;
    if (!r || !i || !i->id || !*i->id || !i->name || !ops)
    {
        return VM_INVALID;
    }
    if (find(r, i->id))
    {
        return VM_DUPLICATE;
    }
    n = calloc(1, sizeof(*n));
    if (!n)
    {
        return VM_NOMEM;
    }
    n->info = *i;
    n->info.id = copy(i->id);
    n->info.name = copy(i->name);
    n->ops = ops;
    if (!n->info.id || !n->info.name)
    {
        free((void *)n->info.id);
        free((void *)n->info.name);
        free(n);
        return VM_NOMEM;
    }
    INIT_LIST_HEAD(&n->link);
    LIST_ADD_TAIL(&n->link, &r->head);
    r->count++;
    return VM_OK;
}
/**
 * 函数说明：vm_registry_remove，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_remove(vm_registry_t *r, const char *id)
{
    node *n = find(r, id);
    if (!r || !id)
    {
        return VM_INVALID;
    }
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (n->refs)
    {
        return VM_BUSY;
    }
    LIST_DEL(&n->link);
    free((void *)n->info.id);
    free((void *)n->info.name);
    free(n);
    r->count--;
    return VM_OK;
}
/**
 * 函数说明：vm_registry_destroy，销毁对象并释放相关资源。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_destroy(vm_registry_t *r)
{
    list_head_t *p;
    if (!r)
    {
        return VM_OK;
    }
    for (p = r->head.next; p != &r->head; p = p->next)
    {
        if (list_entry(p, node, link)->refs)
        {
            return VM_BUSY;
        }
    }
    while (!LIST_EMPTY(&r->head))
    {
        node *n = list_entry(r->head.next, node, link);
        vm_registry_remove(r, n->info.id);
    }
    free(r);
    return VM_OK;
}
/**
 * 函数说明：vm_registry_count，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_registry_count(const vm_registry_t *r)
{
    return r ? r->count : 0;
}
/**
 * 函数说明：vm_registry_at，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；i：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_registry_at(const vm_registry_t *r, size_t i, vm_adapter_info_t *out)
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
            *out = list_entry(p, node, link)->info;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_registry_acquire，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。；ops：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_registry_acquire(vm_registry_t *r, const char *id, const void **ops)
{
    node *n = find(r, id);
    if (!r || !id || !ops)
    {
        return VM_INVALID;
    }
    *ops = NULL;
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (n->refs == SIZE_MAX)
    {
        return VM_BUSY;
    }
    n->refs++;
    *ops = n->ops;
    return VM_OK;
}
/**
 * 函数说明：vm_registry_release，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；id：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_registry_release(vm_registry_t *r, const char *id)
{
    node *n = find(r, id);
    if (!n)
    {
        return VM_NOT_FOUND;
    }
    if (!n->refs)
    {
        return VM_INVALID;
    }
    n->refs--;
    return VM_OK;
}
