/*
 * 文件说明：ELF/AXF 解析模块内部接口声明，供模块内部源文件使用。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

/**
 * @brief MCU专用链表实现（无动态内存分配，线程安全）
 * @note 适用于资源受限的嵌入式系统，提供毒药指针检测和原子操作支持
 * @warning 毒药地址需根据具体MCU内存映射配置（通常选择非法访问地址）
 */
#ifndef _MCU_LIST_H_
/* 常量说明：_MCU_LIST_H_ 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define _MCU_LIST_H_
#include <stddef.h>

/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct list_head
{
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct list_head *next;
    /* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
    struct list_head *prev;
} list_head_t;

/* 定义调试毒药标记（避免悬空指针访问和重复删除，访问这些地址会触发硬件错误） */
/* 常量说明：LIST_POISON1 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_POISON1 ((void *)0xFFFFFFFF) /* 典型非法地址1 */
/* 常量说明：LIST_POISON2 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_POISON2 ((void *)0xFFFFFFFE) /* 典型非法地址2 */

/* 互斥锁实现（需用户根据RTOS实现） */
/* 常量说明：LIST_LOCK() 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_LOCK()   //disable_irq() /* 关中断实现临界区保护 */
/* 常量说明：LIST_UNLOCK() 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_UNLOCK() //enable_irq()  /* 开中断恢复 */

/**
 * @brief 初始化链表头节点
 * @param ptr 链表头节点指针（必须为有效地址）
 * @note 创建环形空链表，next和prev均指向自身
 */
/* 常量说明：INIT_LIST_HEAD(ptr) 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define INIT_LIST_HEAD(ptr)                                                    \
    do                                                                         \
    {                                                                          \
        (ptr)->next = (ptr);                                                   \
        (ptr)->prev = (ptr);                                                   \
    } while (0)

/**
 * @brief 尾插法添加节点（非线程安全）
 * @param newp 要添加的新节点指针（必须已初始化）
 * @param head 链表头节点指针（必须已初始化）
 * @warning 在中断环境使用时必须配合LIST_LOCK使用
 */
/* 常量说明：LIST_ADD_TAIL(newp, 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_ADD_TAIL(newp, head)                                              \
    do                                                                         \
    {                                                                          \
        (head)->prev->next = (newp);                                           \
        (newp)->next = (head);                                                 \
        (newp)->prev = (head)->prev;                                           \
        (head)->prev = (newp);                                                 \
    } while (0)

/**
 * @brief 线程安全尾插法（带中断保护）
 * @param newp 要添加的新节点指针
 * @param head 链表头节点指针
 * @note 通过关中断保证原子操作，适用于中断与主循环共享链表
 */
/* 常量说明：LIST_ADD_TAIL_SAFE(newp, 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_ADD_TAIL_SAFE(newp, head)                                         \
    do                                                                         \
    {                                                                          \
        LIST_LOCK();                                                           \
        LIST_ADD_TAIL(newp, head);                                             \
        LIST_UNLOCK();                                                         \
    } while (0)

/**
 * @brief 删除链表节点（非线程安全）
 * @param entry 要删除的节点指针
 * @warning 删除后节点指针将被标记为毒药地址，任何访问将导致硬件错误
 */
/* 常量说明：LIST_DEL(entry) 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_DEL(entry)                                                        \
    do                                                                         \
    {                                                                          \
        (entry)->prev->next = (entry)->next;                                   \
        (entry)->next->prev = (entry)->prev;                                   \
        (entry)->next = (list_head_t *)LIST_POISON1;                           \
        (entry)->prev = (list_head_t *)LIST_POISON2;                           \
    } while (0)

/**
 * @brief 通过链表节点获取外层结构体指针
 * @param ptr   链表节点指针
 * @param type  外层结构体类型
 * @param member 链表节点在结构体中的成员名
 * @return 外层结构体指针
 * @note 类似Linux内核的container_of宏，但无编译器扩展依赖
 */
/* 常量说明：list_entry(ptr, 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define list_entry(ptr, type, member)                                          \
    ((type *)((char *)(ptr)-offsetof(type, member)))

/**
 * @brief 遍历链表中的外层结构体（非安全版本）
 * @param type  外层结构体类型
 * @param pos   当前结构体指针（迭代变量）
 * @param head  链表头节点指针
 * @param member 链表节点成员名
 * @warning 遍历期间不能删除当前节点，否则会导致遍历崩溃
 */
/* 常量说明：LIST_FOR_EACH_ENTRY(type, 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_FOR_EACH_ENTRY(type, pos, head, member)                           \
    for (pos = list_entry((head)->next, type, member); &pos->member != (head); \
         pos = list_entry(pos->member.next, type, member))

/**
 * @brief 安全遍历链表中的外层结构体
 * @param type   外层结构体类型
 * @param pos    当前结构体指针（迭代变量）
 * @param n      临时存储下一个节点的指针
 * @param head   链表头节点指针
 * @param member 链表节点成员名
 * @note 允许在遍历过程中删除当前节点
 */
/* 常量说明：LIST_FOR_EACH_ENTRY_SAFE(type, 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_FOR_EACH_ENTRY_SAFE(type, pos, n, head, member)                   \
    for (pos = list_entry((head)->next, type, member),                         \
        n = list_entry(pos->member.next, type, member);                        \
         &pos->member != (head);                                               \
         pos = n, n = list_entry(n->member.next, type, member))

/**
 * @brief 检测链表是否为空（严格版本）
 * @param head 链表头节点指针
 * @return 0-非空，1-空
 * @note 同时检查next和prev指针，避免中间状态误判
 * @warning 需在原子上下文中调用（如关中断状态）
 */
/* 常量说明：LIST_EMPTY(head) 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define LIST_EMPTY(head) ((head)->next == (head) && (head)->prev == (head))

/*
  ---使用示例---

typedef struct __Test
{
	void *Priv;
	int (*TestFunc)(void *arg);
	list_head_t List;
} Test;

static list_head_t DevList;

char data1[6] = "node1";
char data2[6] = "node2";
char data3[6] = "node3";
int test_func(void *arg)
{
	char *buff = (char *)arg;
	printf("---[TEST][%s]---\r\n", buff);
	return 0;
}

static Test node1 = {
	.Priv = data1,
	.TestFunc = test_func
};
static Test node2 = {
	.Priv = data2,
	.TestFunc = test_func
};
static Test node3 = {
	.Priv = data3,
	.TestFunc = test_func
};

int main(void)
{
	INIT_LIST_HEAD(&DevList);

	LIST_ADD_TAIL(&node1.List, &DevList);
	LIST_ADD_TAIL(&node2.List, &DevList);
	LIST_ADD_TAIL_SAFE(&node3.List, &DevList);
	
	Test *pos, *tmp;
	
	if (!LIST_EMPTY(&DevList)) {
		LIST_FOR_EACH_ENTRY(Test, pos, &DevList, List) {
			if (pos->TestFunc) {
				pos->TestFunc(pos->Priv);
			}
		}
		
		LIST_DEL(&node2.List);
		
		LIST_FOR_EACH_ENTRY_SAFE(Test, pos, tmp, &DevList, List) {
			if (pos->TestFunc) {
				pos->TestFunc(pos->Priv);
			}
		}
	}

	LIST_DEL(&node1.List);
	LIST_DEL(&node3.List);
	
	if (LIST_EMPTY(&DevList)) {
		printf("---[TEST][NULL]---\n\r");
	}
}

*/

#endif /* _MCU_LIST_H_ */
