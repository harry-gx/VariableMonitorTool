/*
 * 文件说明：二进制 reader 辅助实现，负责边界安全的数据读取。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_reader.h"
/**
 * 函数说明：vm_reader_range，读取数据或发起读取请求。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；o：函数输入参数，参与本函数的计算、查找或状态更新。；n：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
int vm_reader_range(const vm_reader_t *r, size_t o, size_t n)
{
    return r && (r->data || !r->size) && o <= r->size && n <= r->size - o;
}
/**
 * 函数说明：vm_read_uint，读取数据或发起读取请求。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；o：函数输入参数，参与本函数的计算、查找或状态更新。；w：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_read_uint(const vm_reader_t *r, size_t o, unsigned w, uint64_t *out)
{
    uint64_t v = 0;
    unsigned i;
    if (!out || (w != 1 && w != 2 && w != 4 && w != 8))
    {
        return VM_INVALID;
    }
    if (!vm_reader_range(r, o, w))
    {
        return VM_FORMAT;
    }
    for (i = 0; i < w; i++)
    {
        v |= (uint64_t)r->data[o + i] << (8 * (r->big_endian ? w - 1 - i : i));
    }
    *out = v;
    return VM_OK;
}
