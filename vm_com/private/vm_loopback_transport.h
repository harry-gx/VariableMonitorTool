/*
 * 文件说明：通信模块内部接口声明，供模块内部源文件使用。
 * 所属模块：通信模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_LOOPBACK_TRANSPORT_H
#define VM_LOOPBACK_TRANSPORT_H
#include "vm_transport.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_loopback_transport vm_loopback_transport_t;
/**
 * 函数说明：vm_loopback_transport_create，创建并初始化对象。
 * 输入：out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_loopback_transport_create(vm_loopback_transport_t **out);
/**
 * 函数说明：vm_loopback_transport_destroy，销毁对象并释放相关资源。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_loopback_transport_destroy(vm_loopback_transport_t *transport);
/**
 * 函数说明：vm_loopback_transport_base，执行本模块对应功能逻辑。
 * 输入：transport：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
vm_transport_t *vm_loopback_transport_base(vm_loopback_transport_t *transport);
VM_END
#endif
