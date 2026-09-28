/*
 * 文件说明：ELF/AXF 解析模块内部接口声明，供模块内部源文件使用。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_READER_H
#define VM_READER_H
#include "vm_status.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：data，保存当前对象运行所需的状态、参数或缓存数据。 */
    const uint8_t *data;
    /* 变量说明：size，数据长度，单位为字节。 */
    size_t size;
    /* 变量说明：big_endian，保存当前对象运行所需的状态、参数或缓存数据。 */
    int big_endian;
} vm_reader_t;
/**
 * 函数说明：vm_read_uint，读取数据或发起读取请求。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；width：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_read_uint(const vm_reader_t *r,
                         size_t offset,
                         unsigned width,
                         uint64_t *out);
/**
 * 函数说明：vm_reader_range，读取数据或发起读取请求。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；length：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
int vm_reader_range(const vm_reader_t *r, size_t offset, size_t length);
VM_END
#endif
