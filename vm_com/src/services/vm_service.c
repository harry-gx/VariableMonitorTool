/*
 * 文件说明：Services 层总入口实现，集中注册通信模块支持的协议服务节点。
 * 所属模块：通信模块 / Services 层。
 * 设计要点：参考 tkd_shal 的 AsServiceInit 思路，本文件只维护显式服务注册表；删除表项即可禁用对应协议。
 */

#include "vm_service.h"

#include "vm_custom_service.h"

#include <string.h>

/* 类型说明：Services 支持的协议服务类型。 */
typedef enum
{
    VM_SERVICE_KIND_CUSTOM = 0
} vm_service_kind_t;

/* 类型说明：Services 显式协议节点表项。 */
typedef struct
{
    /* 变量说明：协议节点名称，仅用于代码阅读和调试定位。 */
    const char *name;
    /* 变量说明：协议服务类型，用于在初始化时直接调用对应协议注册接口。 */
    vm_service_kind_t kind;
    /* 变量说明：注册成功后的协议实例指针，由静态协议模块返回。 */
    void *instance;
} vm_service_entry_t;

/* 类型说明：Services 管理器内部结构。 */
struct vm_service_manager
{
    /* 变量说明：路由层控制块，协议服务节点注册到该 PduR 对象。 */
    vm_pdur_t *pdur;
    /* 变量说明：管理器是否已完成注册。 */
    uint8_t initialized;
};

/*
 * 变量说明：Services 层显式协议注册表。
 * 修改说明：删除某个表项后，对应协议不会注册到 PduR，整条协议链路即不可用。
 */
static vm_service_entry_t g_vm_service_table[] =
{
    { "custom", VM_SERVICE_KIND_CUSTOM, NULL }
};

/* 变量说明：Services 静态管理器，通信模块当前只需要一个协议栈实例。 */
static vm_service_manager_t g_vm_service_manager;

vm_status_t vm_service_manager_create(
    vm_pdur_t *pdur,
    vm_pdur_device_type_t device_type,
    uint32_t channel,
    vm_service_manager_t **out_manager)
{
    vm_custom_service_t *custom_service;
    size_t index;
    vm_status_t status;

    if ((out_manager == NULL) || (pdur == NULL))
    {
        status = VM_INVALID;
    }
    else if (g_vm_service_manager.initialized != 0u)
    {
        *out_manager = NULL;
        status = VM_BUSY;
    }
    else
    {
        *out_manager = NULL;
        (void)memset(&g_vm_service_manager, 0, sizeof(g_vm_service_manager));
        g_vm_service_manager.pdur = pdur;
        status = VM_OK;

        for (index = 0u;
             (index < (sizeof(g_vm_service_table) / sizeof(g_vm_service_table[0]))) &&
             (status == VM_OK);
             ++index)
        {
            switch (g_vm_service_table[index].kind)
            {
                case VM_SERVICE_KIND_CUSTOM:
                    custom_service = NULL;
                    status = vm_custom_service_create(pdur,
                                                      device_type,
                                                      channel,
                                                      &custom_service);
                    g_vm_service_table[index].instance = custom_service;
                    break;

                default:
                    status = VM_UNSUPPORTED;
                    break;
            }
        }

        if (status == VM_OK)
        {
            g_vm_service_manager.initialized = 1u;
            *out_manager = &g_vm_service_manager;
        }
        else
        {
            vm_service_manager_destroy(&g_vm_service_manager);
        }
    }

    return status;
}

void vm_service_manager_destroy(vm_service_manager_t *manager)
{
    size_t index;

    if ((manager != NULL) && (manager->initialized != 0u))
    {
        index = sizeof(g_vm_service_table) / sizeof(g_vm_service_table[0]);
        while (index > 0u)
        {
            --index;
            if (g_vm_service_table[index].instance != NULL)
            {
                switch (g_vm_service_table[index].kind)
                {
                    case VM_SERVICE_KIND_CUSTOM:
                        vm_custom_service_destroy(
                            (vm_custom_service_t *)g_vm_service_table[index].instance);
                        break;

                    default:
                        break;
                }
                g_vm_service_table[index].instance = NULL;
            }
        }
        (void)memset(manager, 0, sizeof(*manager));
    }
}
