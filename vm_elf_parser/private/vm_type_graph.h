/*
 * 文件说明：ELF/AXF 解析模块内部接口声明，供模块内部源文件使用。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#ifndef VM_TYPE_GRAPH_H
#define VM_TYPE_GRAPH_H
#include "vm_dwarf.h"
VM_BEGIN
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct vm_type_graph vm_type_graph_t;
/* 类型说明：枚举限定模块状态、事件或设备类型的取值范围。 */
typedef enum
{
    VM_TYPE_UNKNOWN = 0,
    VM_TYPE_BASE,
    VM_TYPE_POINTER,
    VM_TYPE_ARRAY,
    VM_TYPE_STRUCT,
    VM_TYPE_UNION,
    VM_TYPE_ENUM,
    VM_TYPE_ENUMERATOR,
    VM_TYPE_TYPEDEF,
    VM_TYPE_CONST,
    VM_TYPE_VOLATILE,
    VM_TYPE_VARIABLE,
    VM_TYPE_MEMBER,
    VM_TYPE_SUBRANGE
} vm_type_kind_t;
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：die_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t die_offset;
    /* 变量说明：type_die_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t type_die_offset;
    /* 变量说明：parent_die_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t parent_die_offset;
    /* 变量说明：name，变量名、页面名或节点名。 */
    const char *name;
    /* 变量说明：byte_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t byte_size;
    /* 变量说明：kind，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_type_kind_t kind;
    /* 变量说明：has_name，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_name;
    /* 变量说明：has_type，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_type;
    /* 变量说明：has_byte_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_byte_size;
    /* 变量说明：member_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t member_offset;
    /* 变量说明：bit_offset，位字段起始 bit 偏移。 */
    uint64_t bit_offset;
    /* 变量说明：bit_size，位字段 bit 宽度。 */
    uint64_t bit_size;
    /* 变量说明：location_address，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t location_address;
    /* 变量说明：enum_value，保存当前对象运行所需的状态、参数或缓存数据。 */
    int64_t enum_value;
    /* 变量说明：element_count，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint64_t element_count;
    /* 变量说明：has_element_count，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_element_count;
    /* 变量说明：has_stride，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_stride;
    uint8_t has_member_offset; /* zero offset is valid; unknown is not zero */
    /* 变量说明：has_bit_offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_bit_offset;
    /* 变量说明：has_bit_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_bit_size;
    /* 变量说明：bit_offset_is_data，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t bit_offset_is_data;
    /* 变量说明：has_location_address，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_location_address;
    /* 变量说明：has_enum_value，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t has_enum_value;
} vm_type_node_view_t;
/**`n * 说明：类型图快照拥有变量名和类型名内存，生命周期可以超过输入 ELF/section 缓冲区；构建失败时不会发布半成品图。`n * 函数说明：vm_type_graph_build，构建 DWARF 类型图。`n * 输入：sections：DWARF 调试节集合；out：类型图对象输出指针；error：失败时的 DWARF 错误信息。`n * 输出：成功时创建类型图快照并写入 out。`n * 返回：返回 VM_OK 表示成功，其它状态码表示调试信息缺失、格式错误或内存不足。`n */
vm_status_t vm_type_graph_build(const vm_dwarf_sections_t *sections,
                                vm_type_graph_t **out,
                                vm_dwarf_error_t *error);
/**
 * 函数说明：vm_type_graph_destroy，销毁对象并释放相关资源。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_type_graph_destroy(vm_type_graph_t *graph);
/**
 * 函数说明：vm_type_graph_count，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_count(const vm_type_graph_t *graph);
/**
 * 函数说明：vm_type_graph_at，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_at(const vm_type_graph_t *graph,
                             size_t index,
                             vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_find_die，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；die_offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_die(const vm_type_graph_t *graph,
                                   uint64_t die_offset,
                                   vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_variable_count，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_variable_count(const vm_type_graph_t *graph);
/**
 * 函数说明：vm_type_graph_static_address_count，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_static_address_count(const vm_type_graph_t *graph);
/**
 * 函数说明：vm_type_graph_variable_at，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_variable_at(const vm_type_graph_t *graph,
                                      size_t index,
                                      vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_find_variable，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_variable(const vm_type_graph_t *graph,
                                        const char *name,
                                        vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_find_variable_by_address，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_variable_by_address(const vm_type_graph_t *graph,
                                                   uint64_t address,
                                                   vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_find_variable_containing，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_variable_containing(const vm_type_graph_t *graph,
                                                   uint64_t address,
                                                   vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_member_count，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_die：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_member_count(const vm_type_graph_t *graph,
                                  uint64_t aggregate_die);
/**
 * 函数说明：vm_type_graph_member_at，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_die：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_member_at(const vm_type_graph_t *graph,
                                    uint64_t aggregate_die,
                                    size_t index,
                                    vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_find_member_containing，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；aggregate_die：函数输入参数，参与本函数的计算、查找或状态更新。；offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_member_containing(const vm_type_graph_t *graph,
                                                 uint64_t aggregate_die,
                                                 uint64_t offset,
                                                 vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_find_enumerator，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_enumerator(const vm_type_graph_t *graph,
                                          uint64_t enum_die,
                                          const char *name,
                                          vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_find_enumerator_by_value，查找匹配对象。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。；value：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_find_enumerator_by_value(const vm_type_graph_t *graph,
                                                   uint64_t enum_die,
                                                   int64_t value,
                                                   vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_enumerator_count，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_type_graph_enumerator_count(const vm_type_graph_t *graph,
                                      uint64_t enum_die);
/**
 * 函数说明：vm_type_graph_enumerator_at，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；enum_die：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_enumerator_at(const vm_type_graph_t *graph,
                                        uint64_t enum_die,
                                        size_t index,
                                        vm_type_node_view_t *out);
enum
{
    VM_TYPE_QUAL_CONST = 1,
    VM_TYPE_QUAL_VOLATILE = 2
};
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：type，对象类型或事件类型。 */
    vm_type_node_view_t type;
    /* 变量说明：qualifiers，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t qualifiers;
} vm_type_resolution_t;
/* Follow variable/typedef/const/volatile only. Stop at pointers and aggregates;
   qualifiers describe this level, not a pointer's pointee. Outputs unchanged
   on failure: FORMAT=cycle/invalid edge, NOT_FOUND=missing node,
   UNSUPPORTED=missing type or size. sizeof also infers contiguous fixed-array sizes; overflow returns FORMAT. */
vm_status_t vm_type_graph_resolve(const vm_type_graph_t *graph,
                                  uint64_t die_offset,
                                  vm_type_resolution_t *out);
/**
 * 函数说明：vm_type_graph_sizeof，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；die_offset：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_sizeof(const vm_type_graph_t *graph,
                                 uint64_t die_offset,
                                 uint64_t *out);
/* Array DIE (after resolve), direct dimensions in declaration order.
   Unknown/dynamic dimensions remain visible with has_element_count == 0. */
size_t vm_type_graph_dimension_count(const vm_type_graph_t *graph,
                                     uint64_t array_die);
/**
 * 函数说明：vm_type_graph_dimension_at，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；array_die：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_dimension_at(const vm_type_graph_t *graph,
                                       uint64_t array_die,
                                       size_t index,
                                       vm_type_node_view_t *out);
/**
 * 函数说明：vm_type_graph_array_count，执行本模块对应功能逻辑。
 * 输入：graph：函数输入参数，参与本函数的计算、查找或状态更新。；array_die：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_type_graph_array_count(const vm_type_graph_t *graph,
                                      uint64_t array_die,
                                      uint64_t *out);
VM_END
#endif
