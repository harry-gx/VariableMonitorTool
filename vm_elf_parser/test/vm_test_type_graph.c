#include "vm_type_graph.h"
#include <stdio.h>
#include <string.h>
#define C(x)                                                                   \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            fprintf(stderr, "type graph test line %d: %s\n", __LINE__, #x);    \
            return 1;                                                          \
        }                                                                      \
    } while (0)
static void put(unsigned char *b, size_t p, unsigned v, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++)
    {
        b[p + i] = (unsigned char)(v >> (8 * i));
    }
}
static int members(void)
{
    unsigned char a[] = {1, 0x11, 1,    0, 0, 2, 0x13, 1, 0, 0, 3, 0x0d,
                         0, 0x38, 0x0b, 0, 0, 4, 0x0d, 0, 0, 0, 0};
    unsigned char b[32] = {0};
    vm_dwarf_sections_t s = {{b, 23}, {a, sizeof(a)}, {NULL, 0}, {NULL, 0}, 0};
    vm_type_graph_t *g = NULL;
    vm_type_node_view_t v;
    put(b, 0, 19, 4);
    put(b, 4, 4, 2);
    b[10] = 4;
    /* CU -> struct A -> member@0, nested struct -> member@8; struct B -> unknown. */
    b[11] = 1;
    b[12] = 2;
    b[13] = 3;
    b[14] = 0;
    b[15] = 2;
    b[16] = 3;
    b[17] = 8;
    b[18] = 0;
    b[19] = 0;
    b[20] = 2;
    b[21] = 4;
    /* Add null terminators for B and CU. */
    b[22] = 0;
    b[23] = 0;
    s.info.size = 24;
    put(b, 0, 20, 4);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_member_count(g, 12) == 1);
    C(vm_type_graph_member_count(g, 15) == 1);
    C(vm_type_graph_member_count(g, 20) == 1);
    C(vm_type_graph_member_at(g, 12, 0, &v) == VM_OK && v.die_offset == 13);
    C(v.has_member_offset && v.member_offset == 0);
    C(vm_type_graph_member_at(g, 15, 0, &v) == VM_OK && v.member_offset == 8);
    C(vm_type_graph_member_at(g, 20, 0, &v) == VM_OK && !v.has_member_offset);
    C(vm_type_graph_member_at(g, 12, 1, &v) == VM_NOT_FOUND);
    vm_type_graph_destroy(g);
    return 0;
}
static int arrays(void)
{
    unsigned char a[] = {1,    0x11, 1,    0x13, 0x0b, 0,    0,    2,    0x24,
                         0,    0x0b, 0x0b, 0,    0,    3,    1,    1,    0x49,
                         0x13, 0,    0,    4,    0x21, 0,    0x2f, 0x0b, 0,
                         0,    5,    0x21, 0,    0x37, 7,    0,    0,    6,
                         0x21, 0,    0x22, 0x0d, 0x2f, 0x0d, 0,    0,    7,
                         0x21, 0,    0x37, 0x0b, 0x51, 0x0b, 0,    0,    0};
    unsigned char b[64] = {0};
    unsigned char saved[64];
    vm_dwarf_sections_t s = {{b, 25}, {a, sizeof(a)}, {NULL, 0}, {NULL, 0}, 0};
    vm_type_graph_t *g = NULL;
    uint64_t n = 777;
    vm_type_node_view_t d;
    put(b, 0, 21, 4);
    put(b, 4, 4, 2);
    b[10] = 4;
    b[11] = 1;
    b[12] = 1;
    b[13] = 2;
    b[14] = 4;
    b[15] = 3;
    put(b, 16, 13, 4);
    b[20] = 4;
    b[21] = 2;
    b[22] = 0;
    b[23] = 0;
    s.info.size = 24;
    put(b, 0, 20, 4);
    memcpy(saved, b, sizeof(b));
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_dimension_count(g, 15) == 1);
    C(vm_type_graph_dimension_at(g, 15, 0, &d) == VM_OK &&
      d.has_element_count && d.element_count == 3);
    C(vm_type_graph_array_count(g, 15, &n) == VM_OK && n == 3);
    C(vm_type_graph_sizeof(g, 15, &n) == VM_OK && n == 12);
    vm_type_graph_destroy(g);
    /* Nested array [2][3], following an element array without recursion. */
    b[23] = 3;
    put(b, 24, 15, 4);
    b[28] = 4;
    b[29] = 1;
    b[30] = 0;
    b[31] = 0;
    s.info.size = 32;
    put(b, 0, 28, 4);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_sizeof(g, 23, &n) == VM_OK && n == 24);
    vm_type_graph_destroy(g);
    memcpy(b, saved, sizeof(b));
    s.info.size = 24;
    /* No language means no guessed lower bound. */
    b[12] = 0;
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    n = 777;
    C(vm_type_graph_sizeof(g, 15, &n) == VM_UNSUPPORTED && n == 777);
    vm_type_graph_destroy(g);
    memcpy(b, saved, sizeof(b));
    b[20] = 6;
    b[21] = 0x7e;
    b[22] = 1;
    b[23] = 0;
    b[24] = 0;
    s.info.size = 25;
    put(b, 0, 21, 4);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_sizeof(g, 15, &n) == VM_OK && n == 16);
    vm_type_graph_destroy(g);
    /* Two dimensions: 3 * 5 elements. */
    memcpy(b, saved, sizeof(b));
    b[22] = 4;
    b[23] = 4;
    b[24] = 0;
    b[25] = 0;
    s.info.size = 26;
    put(b, 0, 22, 4);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_dimension_count(g, 15) == 2);
    C(vm_type_graph_sizeof(g, 15, &n) == VM_OK && n == 60);
    vm_type_graph_destroy(g);
    /* Explicit stride is not silently treated as contiguous. */
    b[20] = 7;
    b[21] = 3;
    b[22] = 8;
    b[23] = 0;
    b[24] = 0;
    s.info.size = 25;
    put(b, 0, 21, 4);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    n = 777;
    C(vm_type_graph_sizeof(g, 15, &n) == VM_UNSUPPORTED && n == 777);
    vm_type_graph_destroy(g);
    /* Count fits uint64, byte size does not. */
    memcpy(b, saved, sizeof(b));
    b[20] = 5;
    memset(b + 21, 255, 8);
    b[29] = 0;
    b[30] = 0;
    s.info.size = 31;
    put(b, 0, 27, 4);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_array_count(g, 15, &n) == VM_OK && n == UINT64_MAX);
    n = 777;
    C(vm_type_graph_sizeof(g, 15, &n) == VM_FORMAT && n == 777);
    vm_type_graph_destroy(g);
    /* Empty array and cyclic element type. */
    memset(b + 21, 0, 8);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_sizeof(g, 15, &n) == VM_OK && n == 0);
    vm_type_graph_destroy(g);
    put(b, 16, 15, 4);
    C(vm_type_graph_build(&s, &g, NULL) == VM_OK);
    C(vm_type_graph_sizeof(g, 15, &n) == VM_FORMAT);
    vm_type_graph_destroy(g);
    return 0;
}
int main(void)
{
    unsigned char abbrev[] = {1,    0x11, 1,    0, 0,    2,    0x24, 0,    3,
                              8,    0x0b, 0x0b, 0, 0,    3,    0x16, 0,    3,
                              8,    0x49, 0x13, 0, 0,    4,    0x26, 0,    0x49,
                              0x13, 0,    0,    5, 0x35, 0,    0x49, 0x13, 0,
                              0,    6,    0x34, 0, 3,    8,    0x49, 0x13, 0,
                              0,    7,    0x0f, 0, 0x0b, 0x0b, 0x49, 0x13, 0,
                              0,    8,    0x13, 0, 0,    0,    0};
    unsigned char info[128] = {0};
    unsigned char original[128];
    size_t p = 11;
    size_t base;
    size_t alias;
    size_t alias_ref;
    size_t con;
    size_t vol;
    size_t var;
    size_t ptr;
    size_t unknown;
    size_t zero;
    vm_dwarf_sections_t sections = {
        {info, 0}, {abbrev, sizeof(abbrev)}, {NULL, 0}, {NULL, 0}, 0};
    vm_type_graph_t *g = NULL;
    vm_type_resolution_t r;
    vm_type_node_view_t v;
    uint64_t size;
    info[p++] = 1;
    base = p;
    info[p++] = 2;
    memcpy(info + p, "int", 4);
    p += 4;
    info[p++] = 4;
    alias = p;
    info[p++] = 3;
    memcpy(info + p, "Alias", 6);
    p += 6;
    alias_ref = p;
    put(info, p, (unsigned)base, 4);
    p += 4;
    con = p;
    info[p++] = 4;
    put(info, p, (unsigned)alias, 4);
    p += 4;
    vol = p;
    info[p++] = 5;
    put(info, p, (unsigned)con, 4);
    p += 4;
    var = p;
    info[p++] = 6;
    memcpy(info + p, "value", 6);
    p += 6;
    put(info, p, (unsigned)vol, 4);
    p += 4;
    ptr = p;
    info[p++] = 7;
    info[p++] = 4;
    put(info, p, (unsigned)con, 4);
    p += 4;
    unknown = p;
    info[p++] = 8;
    zero = p;
    info[p++] = 2;
    info[p++] = 0;
    info[p++] = 0;
    info[p++] = 0;
    put(info, 0, (unsigned)p - 4, 4);
    put(info, 4, 4, 2);
    info[10] = 4;
    sections.info.size = p;
    memcpy(original, info, sizeof(info));
    C(vm_type_graph_build(&sections, &g, NULL) == VM_OK);
    memset(info,
           0xcc,
           sizeof(info)); /* Borrowed buffers can now disappear/change. */
    C(vm_type_graph_find_die(g, alias, &v) == VM_OK &&
      !strcmp(v.name, "Alias"));
    C(vm_type_graph_variable_count(g) == 1);
    C(vm_type_graph_resolve(g, var, &r) == VM_OK);
    C(r.type.die_offset == base &&
      r.qualifiers == (VM_TYPE_QUAL_CONST | VM_TYPE_QUAL_VOLATILE));
    C(vm_type_graph_sizeof(g, var, &size) == VM_OK && size == 4);
    C(vm_type_graph_resolve(g, ptr, &r) == VM_OK &&
      r.type.kind == VM_TYPE_POINTER && r.qualifiers == 0);
    C(vm_type_graph_sizeof(g, ptr, &size) == VM_OK && size == 4);
    size = 777;
    C(vm_type_graph_sizeof(g, unknown, &size) == VM_UNSUPPORTED && size == 777);
    C(vm_type_graph_sizeof(g, zero, &size) == VM_OK && size == 0);
    C(vm_type_graph_find_die(g, 999, &v) == VM_NOT_FOUND);
    vm_type_graph_destroy(g);
    memcpy(info, original, sizeof(info));
    put(info, alias_ref, (unsigned)alias, 4);
    C(vm_type_graph_build(&sections, &g, NULL) == VM_OK);
    size = 777;
    C(vm_type_graph_sizeof(g, var, &size) == VM_FORMAT && size == 777);
    vm_type_graph_destroy(g);
    memcpy(info, original, sizeof(info));
    put(info, alias_ref, 11, 4); /* Valid DIE, not a type node. */
    C(vm_type_graph_build(&sections, &g, NULL) == VM_OK);
    C(vm_type_graph_resolve(g, var, &r) == VM_NOT_FOUND);
    vm_type_graph_destroy(g);
    memcpy(info, original, sizeof(info));
    put(info, alias_ref, (unsigned)var, 4);
    C(vm_type_graph_build(&sections, &g, NULL) == VM_OK);
    C(vm_type_graph_resolve(g, alias, &r) == VM_FORMAT);
    vm_type_graph_destroy(g);
    memcpy(info, original, sizeof(info));
    info[p - 1] = 9;
    g = NULL;
    C(vm_type_graph_build(&sections, &g, NULL) != VM_OK && g == NULL);
    C(members() == 0);
    C(arrays() == 0);
    puts(
        "Type graph ownership, qualifier, size, cycle, member and array tests "
        "passed.");
    return 0;
}
