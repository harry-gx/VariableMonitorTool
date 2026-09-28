/*
 * 文件说明：ELF 解析模块命令行入口，用于独立验证变量解析输出。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_monitor_variables.h"

#include <inttypes.h>
#include <stdio.h>

/**
 * 函数说明：vm_cli_flag_text，执行本模块对应功能逻辑。
 * 输入：flag：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static const char *vm_cli_flag_text(uint8_t flag)
{
    const char *text;

    if (flag != 0u)
    {
        text = "yes";
    }
    else
    {
        text = "no";
    }

    return text;
}

/**
 * 函数说明：vm_cli_print_usage，执行本模块对应功能逻辑。
 * 输入：无。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
static void vm_cli_print_usage(void)
{
    (void)fprintf(stderr, "Usage: vm_elf_parser_cli file\n");
}

/**
 * 函数说明：vm_cli_dump_variables，执行本模块对应功能逻辑。
 * 输入：path：待加载的文件路径。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
static int vm_cli_dump_variables(const char *path)
{
    vm_monitor_variable_list_t *list;
    vm_monitor_variable_t variable;
    char error[VM_MONITOR_ERROR_MAX];
    vm_status_t status;
    size_t count;
    size_t index;
    int result;

    list = NULL;
    error[0] = '\0';
    result = 1;

    status = vm_monitor_variables_load(path, &list, error, sizeof(error));
    if (status != VM_OK)
    {
        (void)fprintf(stderr,
                      "load failed: status=%d error=%s\n",
                      (int)status,
                      error);
    }
    else
    {
        count = vm_monitor_variable_count(list);
        (void)printf("name,address,size,type,monitorable,calibratable,writable,bit_field,bit_offset,bit_size\n");

        for (index = 0u; index < count; ++index)
        {
            status = vm_monitor_variable_at(list, index, &variable);
            if (status == VM_OK)
            {
                (void)printf("%s,0x%08" PRIx64 ",%" PRIu64 ",%s,%s,%s,%s,%s,%u,%u\n",
                             variable.name,
                             variable.address,
                             variable.size,
                             variable.type_name,
                             vm_cli_flag_text(variable.monitorable),
                             vm_cli_flag_text(variable.calibratable),
                             vm_cli_flag_text(variable.writable),
                             vm_cli_flag_text(variable.bit_field),
                             (unsigned int)variable.bit_offset,
                             (unsigned int)variable.bit_size);
            }
        }

        result = 0;
    }

    vm_monitor_variable_list_destroy(list);
    return result;
}

/**
 * 函数说明：main，执行本模块对应功能逻辑。
 * 输入：argc：命令行参数数量。；argv：命令行参数数组。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
int main(int argc, char **argv)
{
    int result;

    if (argc != 2)
    {
        vm_cli_print_usage();
        result = 2;
    }
    else
    {
        result = vm_cli_dump_variables(argv[1]);
    }

    return result;
}
