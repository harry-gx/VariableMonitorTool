/*
 * 文件说明：ELF 文件读取实现，负责解析文件头、节表、字符串表和调试节。
 * 所属模块：ELF/AXF 解析模块。
 * 设计要点：正式业务逻辑集中在本文件或本模块内，测试代码位于 test 目录，第三方厂商头文件不在本次注释范围内。
 */

#include "vm_elf.h"
#include "vm_reader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
struct vm_elf
{
    /* 变量说明：bytes，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint8_t *bytes;
    /* 变量说明：symbols，保存当前对象运行所需的状态、参数或缓存数据。 */
    vm_symbol_t *symbols;
    /* 变量说明：count，当前元素数量。 */
    size_t count;
    /* 变量说明：capacity，动态数组容量。 */
    size_t capacity;
    /* 变量说明：has_debug，保存当前对象运行所需的状态、参数或缓存数据。 */
    int has_debug;
    /* 变量说明：file_size，保存当前对象运行所需的状态、参数或缓存数据。 */
    size_t file_size;
};
vm_status_t vm_elf_parse(const void *, size_t, vm_elf_t **, vm_parse_error_t *);
/* 类型说明：结构体保存模块状态、配置、变量描述或解析结果。 */
typedef struct
{
    /* 变量说明：type，对象类型或事件类型。 */
    uint32_t type;
    /* 变量说明：flags，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t flags;
    /* 变量说明：offset，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t offset;
    /* 变量说明：size，数据长度，单位为字节。 */
    uint32_t size;
    /* 变量说明：link，链表节点，用于挂接到注册表或路由表。 */
    uint32_t link;
    /* 变量说明：entry，保存当前对象运行所需的状态、参数或缓存数据。 */
    uint32_t entry;
} section;
/**
 * 函数说明：u，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；o：函数输入参数，参与本函数的计算、查找或状态更新。；w：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static uint32_t u(const vm_reader_t *r, size_t o, unsigned w)
{
    uint64_t v = 0;
    (void)vm_read_uint(r, o, w, &v);
    return (uint32_t)v;
}

/**
 * 函数说明：vm_elf_parse_file，解析输入数据并生成内部结果。
 * 输入：path：待加载的文件路径。；out：输出对象或结果指针，函数成功时写入有效值。；err：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_elf_parse_file(const char *path, vm_elf_t **out, vm_parse_error_t *err)
{
    FILE *f;
    long length;
    uint8_t *data;
    size_t n;
    vm_status_t s;
    if (err)
    {
        err->offset = 0;
        err->message = "";
    }
    if (!path || !out)
    {
        return VM_INVALID;
    }
    *out = NULL;
    f = fopen(path, "rb");
    if (!f)
    {
        if (err)
        {
            err->message = "cannot open file";
        }
        return VM_IO;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (length = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        if (err)
        {
            err->message = "cannot determine file size";
        }
        return VM_IO;
    }
    if ((unsigned long)length > 512UL * 1024UL * 1024UL)
    {
        fclose(f);
        if (err)
        {
            err->message = "file exceeds parser limit";
        }
        return VM_UNSUPPORTED;
    }
    n = (size_t)length;
    data = (uint8_t *)malloc(n ? n : 1);
    if (!data)
    {
        fclose(f);
        return VM_NOMEM;
    }
    if ((n && fread(data, 1, n, f) != n) || ferror(f))
    {
        free(data);
        fclose(f);
        if (err)
        {
            err->message = "file read failed";
        }
        return VM_IO;
    }
    fclose(f);
    s = vm_elf_parse(data, n, out, err);
    free(data);
    return s;
}
/**
 * 函数说明：sec，执行本模块对应功能逻辑。
 * 输入：r：函数输入参数，参与本函数的计算、查找或状态更新。；table：函数输入参数，参与本函数的计算、查找或状态更新。；index：列表索引或数组下标。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回执行结果，具体含义由调用方按接口约定解释。
 */
static section sec(const vm_reader_t *r, size_t table, size_t index)
{
    size_t o = table + index * 40;
    section s;
    s.type = u(r, o + 4, 4);
    s.flags = u(r, o + 8, 4);
    s.offset = u(r, o + 16, 4);
    s.size = u(r, o + 20, 4);
    s.link = u(r, o + 24, 4);
    s.entry = u(r, o + 36, 4);
    return s;
}
/**
 * 函数说明：vm_elf_close，关闭底层资源。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：无返回值。
 */
void vm_elf_close(vm_elf_t *e)
{
    if (e)
    {
        free(e->symbols);
        free(e->bytes);
        free(e);
    }
}
/**
 * 函数说明：vm_elf_section，执行本模块对应功能逻辑。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_elf_section(const vm_elf_t *e, const char *name, vm_elf_section_view_t *out)
{
    vm_reader_t r;
    size_t table;
    size_t n;
    size_t i;
    section names;
    if (!e || !name || !out)
    {
        return VM_INVALID;
    }
    r.data = e->bytes;
    r.size = e->file_size;
    r.big_endian = e->bytes[5] == 2;
    table = u(&r, 32, 4);
    n = u(&r, 48, 2);
    if (!n || !u(&r, 50, 2))
    {
        return VM_NOT_FOUND;
    }
    names = sec(&r, table, u(&r, 50, 2));
    for (i = 1; i < n; i++)
    {
        uint32_t off = u(&r, table + i * 40, 4);
        if (!strcmp((const char *)r.data + names.offset + off, name))
        {
            section s = sec(&r, table, i);
            if (s.type == 8 || (s.flags & 0x800u) ||
                !strncmp(name, ".zdebug_", 8))
            {
                return VM_UNSUPPORTED;
            }
            out->data = r.data + s.offset;
            out->size = s.size;
            out->flags = s.flags;
            out->big_endian = r.big_endian;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_elf_has_debug_info，执行本模块对应功能逻辑。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回整数结果，具体含义由调用场景决定。
 */
int vm_elf_has_debug_info(const vm_elf_t *e)
{
    return e ? e->has_debug : 0;
}
/**
 * 函数说明：vm_elf_symbol_count，执行本模块对应功能逻辑。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_elf_symbol_count(const vm_elf_t *e)
{
    return e ? e->count : 0;
}
/**
 * 函数说明：vm_elf_symbol_at，执行本模块对应功能逻辑。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。；i：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_elf_symbol_at(const vm_elf_t *e, size_t i, vm_symbol_t *out)
{
    if (!e || !out)
    {
        return VM_INVALID;
    }
    if (i >= e->count)
    {
        return VM_NOT_FOUND;
    }
    *out = e->symbols[i];
    return VM_OK;
}
/**
 * 函数说明：vm_elf_variable_count，执行本模块对应功能逻辑。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回数量、长度或索引值。
 */
size_t vm_elf_variable_count(const vm_elf_t *e)
{
    return e ? e->count : 0;
}
/**
 * 函数说明：vm_elf_variable_at，执行本模块对应功能逻辑。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。；i：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t
vm_elf_variable_at(const vm_elf_t *e, size_t i, vm_variable_view_t *out)
{
    vm_symbol_t s;
    if (!out)
    {
        return VM_INVALID;
    }
    if (vm_elf_symbol_at(e, i, &s) != VM_OK)
    {
        return VM_NOT_FOUND;
    }
    out->name = s.name;
    out->address = s.address;
    out->size = s.size;
    out->section_index = s.section_index;
    out->binding = s.binding;
    out->readable = (uint8_t)((s.section_flags & 2u) != 0);
    out->writable = (uint8_t)((s.section_flags & 3u) == 3u);
    out->volatile_hint = 0;
    return VM_OK;
}
/**
 * 函数说明：vm_elf_find_variable_by_address，查找匹配对象。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。；address：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_elf_find_variable_by_address(const vm_elf_t *e,
                                            uint64_t address,
                                            vm_variable_view_t *out)
{
    size_t i;
    vm_variable_view_t v;
    if (!e || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < e->count; i++)
    {
        vm_elf_variable_at(e, i, &v);
        if (address >= v.address && address - v.address < v.size)
        {
            *out = v;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_elf_find_variable，查找匹配对象。
 * 输入：e：函数输入参数，参与本函数的计算、查找或状态更新。；name：函数输入参数，参与本函数的计算、查找或状态更新。；out：输出对象或结果指针，函数成功时写入有效值。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_elf_find_variable(const vm_elf_t *e,
                                 const char *name,
                                 vm_variable_view_t *out)
{
    size_t i;
    vm_variable_view_t v;
    if (!e || !name || !out)
    {
        return VM_INVALID;
    }
    for (i = 0; i < e->count; i++)
    {
        vm_elf_variable_at(e, i, &v);
        if (!strcmp(v.name, name))
        {
            *out = v;
            return VM_OK;
        }
    }
    return VM_NOT_FOUND;
}
/**
 * 函数说明：vm_elf_parse，解析输入数据并生成内部结果。
 * 输入：bytes：函数输入参数，参与本函数的计算、查找或状态更新。；size：数据长度或缓冲区容量。；out：输出对象或结果指针，函数成功时写入有效值。；err：函数输入参数，参与本函数的计算、查找或状态更新。
 * 输出：通过返回值、对象成员或输出参数反馈处理结果。
 * 返回：返回 VM_OK 表示成功，其它状态码表示参数错误、资源不足、忙碌或底层失败。
 */
vm_status_t vm_elf_parse(const void *bytes,
                         size_t size,
                         vm_elf_t **out,
                         vm_parse_error_t *err)
{
    vm_reader_t r;
    vm_elf_t *e = NULL;
    size_t table;
    size_t n;
    size_t i;
    size_t j;
    vm_status_t status = VM_FORMAT;
    size_t bad = 0;
    const char *why = "invalid ELF header";
    if (err)
    {
        err->offset = 0;
        err->message = "";
    }
    if (!out || (!bytes && size))
    {
        return VM_INVALID;
    }
    *out = NULL;
    r.data = bytes;
    r.size = size;
    r.big_endian = 0;
/* 常量说明：FAIL(code, 用于配置协议长度、默认参数、缓冲区容量或编译开关。 */
#define FAIL(code, off, msg)                                                   \
    do                                                                         \
    {                                                                          \
        status = (code);                                                       \
        bad = (off);                                                           \
        why = (msg);                                                           \
        goto fail;                                                             \
    } while (0)
    if (size < 52 || memcmp(bytes, "\177ELF", 4))
    {
        goto fail;
    }
    if (r.data[4] != 1)
    {
        FAIL(VM_UNSUPPORTED, 4, "only ELF32 is implemented");
    }
    if (r.data[5] != 1 && r.data[5] != 2)
    {
        FAIL(VM_FORMAT, 5, "invalid byte order");
    }
    r.big_endian = r.data[5] == 2;
    if (r.data[6] != 1 || u(&r, 20, 4) != 1 || u(&r, 40, 2) != 52)
    {
        goto fail;
    }
    if (u(&r, 18, 2) != 40 || u(&r, 16, 2) != 2)
    {
        FAIL(VM_UNSUPPORTED, 16, "requires linked ARM executable");
    }
    table = u(&r, 32, 4);
    n = u(&r, 48, 2);
    if ((!n && table) || u(&r, 50, 2) == 65535 || u(&r, 44, 2) == 65535)
    {
        FAIL(VM_UNSUPPORTED, 32, "extended ELF numbering not implemented");
    }
    if (n && (table < 52 || u(&r, 46, 2) != 40 ||
              !vm_reader_range(&r, table, n * 40)))
    {
        FAIL(VM_FORMAT, 32, "section table outside file");
    }
    if (!n && u(&r, 50, 2))
    {
        FAIL(VM_FORMAT, 50, "invalid section name index");
    }
    if (n && u(&r, 50, 2) >= n)
    {
        FAIL(VM_FORMAT, 50, "invalid section name index");
    }
    if (u(&r, 44, 2))
    {
        size_t ph = u(&r, 28, 4);
        size_t pn = u(&r, 44, 2);
        if (u(&r, 42, 2) != 32 || !vm_reader_range(&r, ph, pn * 32))
        {
            FAIL(VM_FORMAT, 28, "invalid program table");
        }
        for (i = 0; i < pn; i++)
        {
            size_t o = ph + i * 32;
            uint32_t fs = u(&r, o + 16, 4);
            if (!vm_reader_range(&r, u(&r, o + 4, 4), fs))
            {
                FAIL(VM_FORMAT, o, "segment outside file");
            }
            if (u(&r, o, 4) == 1 && fs > u(&r, o + 20, 4))
            {
                FAIL(VM_FORMAT, o, "segment filesz exceeds memsz");
            }
        }
    }
    for (i = 1; i < n; i++)
    {
        section s = sec(&r, table, i);
        if (s.type != 8 && !vm_reader_range(&r, s.offset, s.size))
        {
            FAIL(VM_FORMAT, table + i * 40, "section outside file");
        }
    }
    e = calloc(1, sizeof(*e));
    if (!e)
    {
        FAIL(VM_NOMEM, 0, "allocation failed");
    }
    e->bytes = malloc(size);
    if (!e->bytes)
    {
        FAIL(VM_NOMEM, 0, "allocation failed");
    }
    memcpy(e->bytes, bytes, size);
    e->file_size = size;
    if (n && u(&r, 50, 2))
    {
        section names = sec(&r, table, u(&r, 50, 2));
        if (names.type != 3 || !names.size || r.data[names.offset] != 0)
        {
            FAIL(VM_FORMAT, names.offset, "invalid section name table");
        }
        for (i = 1; i < n; i++)
        {
            uint32_t off = u(&r, table + i * 40, 4);
            const char *name;
            if (off >= names.size ||
                !memchr(r.data + names.offset + off, 0, names.size - off))
            {
                FAIL(VM_FORMAT, table + i * 40, "invalid section name");
            }
            name = (const char *)r.data + names.offset + off;
            if (!strcmp(name, ".debug_info") || !strcmp(name, ".zdebug_info"))
            {
                e->has_debug = sec(&r, table, i).size != 0;
            }
        }
    }
    for (i = 1; i < n; i++)
    {
        section s = sec(&r, table, i);
        section str;
        if (s.type != 2 && s.type != 11)
        {
            continue;
        }
        if (s.entry != 16 || s.size % 16 || s.link >= n)
        {
            FAIL(VM_FORMAT, table + i * 40, "invalid symbol table");
        }
        str = sec(&r, table, s.link);
        if (str.type != 3 || !str.size || r.data[str.offset] != 0)
        {
            FAIL(VM_FORMAT, str.offset, "invalid string table");
        }
        for (j = 0; j < s.size / 16; j++)
        {
            size_t o = s.offset + j * 16;
            uint32_t name = u(&r, o, 4);
            uint32_t idx = u(&r, o + 14, 2);
            uint8_t info = r.data[o + 12];
            vm_symbol_t v;
            if (name >= str.size ||
                !memchr(r.data + str.offset + name, 0, str.size - name))
            {
                FAIL(VM_FORMAT, o, "unterminated symbol name");
            }
            if (idx == 65535)
            {
                FAIL(VM_UNSUPPORTED, o, "extended symbol index");
            }
            if (idx && idx < 0xff00 && idx >= n)
            {
                FAIL(VM_FORMAT, o, "invalid symbol section");
            }
            if ((info & 15) != 1 || !idx || idx >= 0xff00 || !name)
            {
                continue;
            }
            v.name = (const char *)e->bytes + str.offset + name;
            v.address = u(&r, o + 4, 4);
            v.size = u(&r, o + 8, 4);
            v.section_index = idx;
            v.section_flags = sec(&r, table, idx).flags;
            v.binding = info >> 4;
            if (e->count == e->capacity)
            {
                size_t cap = e->capacity ? e->capacity * 2 : 32;
                vm_symbol_t *p;
                if (cap < e->capacity || cap > SIZE_MAX / sizeof(*p))
                {
                    FAIL(VM_NOMEM, o, "symbol limit");
                }
                p = realloc(e->symbols, cap * sizeof(*p));
                if (!p)
                {
                    FAIL(VM_NOMEM, o, "allocation failed");
                }
                e->symbols = p;
                e->capacity = cap;
            }
            e->symbols[e->count++] = v;
        }
    }
    *out = e;
    return VM_OK;
fail:
    vm_elf_close(e);
    if (err)
    {
        err->offset = bad;
        err->message = why;
    }
    return status;
#undef FAIL
}
