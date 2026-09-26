#include "vm_monitor_variables.h"

#include <inttypes.h>
#include <stdio.h>

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

static void vm_cli_print_usage(void)
{
    (void)fprintf(stderr, "Usage: vm_elf_parser_cli file\n");
}

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