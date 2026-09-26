#include "vm_dwarf.h"
#include "vm_dwarf_values.h"
#include <stdio.h>
#include <string.h>
#define C(x)                                                                   \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            fprintf(stderr, "value test %d: %s\n", __LINE__, #x);              \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void put(unsigned char *b, size_t o, uint64_t v, unsigned w, int be)
{
    unsigned i;
    for (i = 0; i < w; ++i)
    {
        b[o + i] = (unsigned char)(v >> (8 * (be ? w - 1 - i : i)));
    }
}
static int forms(void)
{
    unsigned char b[32] = {0};
    vm_dwarf_form_context_t c = {4, 4, 4, 0, 0};
    vm_dwarf_value_t v;
    vm_dwarf_value_t old;
    size_t next;
    size_t i;
    int be;
    int wide;
    int version;
    struct
    {
        uint64_t form;
        size_t length;
    } fixed[] = {{VM_DW_FORM_data1, 1},
                 {VM_DW_FORM_data2, 2},
                 {VM_DW_FORM_data4, 4},
                 {VM_DW_FORM_data8, 8},
                 {VM_DW_FORM_ref1, 1},
                 {VM_DW_FORM_ref2, 2},
                 {VM_DW_FORM_ref4, 4},
                 {VM_DW_FORM_ref8, 8},
                 {VM_DW_FORM_data16, 16}};
    memset(&old, 0x55, sizeof(old));
    for (i = 0; i < sizeof(fixed) / sizeof(fixed[0]); ++i)
    {
        size_t n;
        for (n = 0; n < fixed[i].length; ++n)
        {
            next = 777;
            v = old;
            C(vm_dwarf_form_read(b, n, 0, fixed[i].form, &c, &v, &next) ==
              VM_FORMAT);
            C(next == 777 && memcmp(&v, &old, sizeof(v)) == 0);
        }
        C(vm_dwarf_form_read(b, sizeof(b), 0, fixed[i].form, &c, &v, &next) ==
          VM_OK);
        C(next == fixed[i].length);
    }
    /* ref_addr depends on version, offset size and target address size. */
    for (be = 0; be < 2; ++be)
    {
        for (wide = 0; wide < 2; ++wide)
        {
            for (version = 2; version <= 5; ++version)
            {
                c.big_endian = be;
                c.offset_size = wide ? 8 : 4;
                c.version = (uint16_t)version;
                c.address_size = 8;
                memset(b, 0, sizeof(b));
                put(b, 0, 0x12345678, version == 2 ? 8 : c.offset_size, be);
                C(vm_dwarf_form_read(
                      b, sizeof(b), 0, VM_DW_FORM_ref_addr, &c, &v, &next) ==
                  VM_OK);
                C(next == (version == 2 ? 8 : c.offset_size) &&
                  v.unsigned_value == 0x12345678);
                C(v.kind == VM_DWARF_INFO_REFERENCE);
                put(b, 0, 0x10203040, c.offset_size, be);
                C(vm_dwarf_form_read(
                      b, sizeof(b), 0, VM_DW_FORM_strp, &c, &v, &next) ==
                  VM_OK);
                C(next == c.offset_size && v.unsigned_value == 0x10203040 &&
                  v.kind == VM_DWARF_STR_OFFSET);
            }
        }
    }
    c.version = 4;
    c.address_size = 4;
    c.offset_size = 4;
    c.big_endian = 0;
    b[0] = 0xe5;
    b[1] = 0x8e;
    b[2] = 0x26;
    C(vm_dwarf_form_read(b, 3, 0, VM_DW_FORM_udata, &c, &v, &next) == VM_OK);
    C(next == 3 && v.unsigned_value == 624485);
    b[0] = 0x9b;
    b[1] = 0xf1;
    b[2] = 0x59;
    C(vm_dwarf_form_read(b, 3, 0, VM_DW_FORM_sdata, &c, &v, &next) == VM_OK);
    C(v.signed_value == -624485 && next == 3);
    memset(b, 0x80, 9);
    b[9] = 0x7f;
    C(vm_dwarf_form_read(b, 10, 0, VM_DW_FORM_sdata, &c, &v, &next) == VM_OK);
    C(v.signed_value == INT64_MIN);
    b[9] = 0x02;
    C(vm_dwarf_form_read(b, 10, 0, VM_DW_FORM_sdata, &c, &v, &next) ==
      VM_FORMAT);
    C(vm_dwarf_form_read(b, 10, 0, VM_DW_FORM_udata, &c, &v, &next) ==
      VM_FORMAT);
    memset(b, 0xff, 10);
    C(vm_dwarf_form_read(b, 10, 0, VM_DW_FORM_udata, &c, &v, &next) ==
      VM_FORMAT);
    b[9] = 1;
    C(vm_dwarf_form_read(b, 10, 0, VM_DW_FORM_udata, &c, &v, &next) == VM_OK &&
      v.unsigned_value == UINT64_MAX);
    b[0] = 0x19;
    C(vm_dwarf_form_read(b, 1, 0, VM_DW_FORM_indirect, &c, &v, &next) == VM_OK);
    C(next == 1 && v.unsigned_value == 1 && v.form == VM_DW_FORM_flag_present);
    C(vm_dwarf_form_read(b, 0, 0, VM_DW_FORM_flag_present, &c, &v, &next) ==
          VM_OK &&
      next == 0);
    b[0] = 255;
    C(vm_dwarf_form_read(b, 1, 0, VM_DW_FORM_flag, &c, &v, &next) == VM_OK &&
      v.unsigned_value == 255);
    b[0] = VM_DW_FORM_ref4;
    put(b, 1, 0x37, 4, 0);
    C(vm_dwarf_form_read(b, 5, 0, VM_DW_FORM_indirect, &c, &v, &next) == VM_OK);
    C(next == 5 && v.kind == VM_DWARF_CU_REFERENCE && v.unsigned_value == 0x37);
    C(vm_dwarf_form_read(b, 4, 0, VM_DW_FORM_indirect, &c, &v, &next) ==
      VM_FORMAT);
    memset(b, VM_DW_FORM_indirect, sizeof(b));
    C(vm_dwarf_form_read(b, sizeof(b), 0, VM_DW_FORM_indirect, &c, &v, &next) ==
      VM_UNSUPPORTED);
    c.version = 5;
    c.implicit_const = -9;
    C(vm_dwarf_form_read(b, 0, 0, VM_DW_FORM_implicit_const, &c, &v, &next) ==
      VM_OK);
    C(next == 0 && v.signed_value == -9);
    b[0] = VM_DW_FORM_implicit_const;
    C(vm_dwarf_form_read(b, 1, 0, VM_DW_FORM_indirect, &c, &v, &next) ==
      VM_FORMAT);
    memcpy(b, "abc", 4);
    C(vm_dwarf_form_read(b, 3, 0, VM_DW_FORM_string, &c, &v, &next) ==
      VM_FORMAT);
    C(vm_dwarf_form_read(b, 4, 0, VM_DW_FORM_string, &c, &v, &next) == VM_OK);
    C(v.size == 3 && next == 4 && !strcmp((const char *)v.data, "abc"));
    for (be = 0; be < 2; ++be)
    {
        unsigned widths[] = {1, 2, 4};
        uint64_t fs[] = {
            VM_DW_FORM_block1, VM_DW_FORM_block2, VM_DW_FORM_block4};
        c.big_endian = be;
        for (i = 0; i < 3; ++i)
        {
            memset(b, 0, sizeof(b));
            put(b, 0, 3, widths[i], be);
            memcpy(b + widths[i], "abc", 3);
            C(vm_dwarf_form_read(b, widths[i] + 3, 0, fs[i], &c, &v, &next) ==
              VM_OK);
            C(next == widths[i] + 3 && v.size == 3 &&
              !memcmp(v.data, "abc", 3));
            C(vm_dwarf_form_read(b, widths[i] + 2, 0, fs[i], &c, &v, &next) ==
              VM_FORMAT);
        }
    }
    b[0] = 3;
    memcpy(b + 1, "abc", 3);
    C(vm_dwarf_form_read(b, 4, 0, VM_DW_FORM_block, &c, &v, &next) == VM_OK &&
      next == 4);
    C(vm_dwarf_form_read(b, 4, 0, VM_DW_FORM_exprloc, &c, &v, &next) == VM_OK &&
      next == 4);
    next = 99;
    C(vm_dwarf_form_read(
          b, sizeof(b), SIZE_MAX, VM_DW_FORM_data4, &c, &v, &next) ==
          VM_FORMAT &&
      next == 99);
    C(vm_dwarf_form_read(b, sizeof(b), 0, 0xdead, &c, &v, &next) ==
      VM_UNSUPPORTED);
    return 0;
}

static int abbreviations(void)
{
    /* Sparse, out-of-order codes; negative implicit const is stored in abbrev. */
    unsigned char a[] = {
        9, 0x24, 0, 0x0b, 0x21, 0x7e, 0, 0, 3, 0x11, 1, 0, 0, 0};
    unsigned char duplicate[] = {1, 0x11, 0, 0, 0, 1, 0x24, 0, 0, 0, 0};
    unsigned char half_end[] = {1, 0x11, 0, 0, 8, 0, 0, 0};
    unsigned char bad_children[] = {1, 0x11, 2, 0, 0, 0};
    unsigned char die[] = {9};
    vm_dwarf_die_header_t h;
    vm_dwarf_attr_form_t af;
    size_t n = 777;
    size_t i;
    C(vm_dwarf_abbrev_count_checked(a, sizeof(a), 0, &n) == VM_OK && n == 2);
    C(vm_dwarf_die_header(die, sizeof(die), 0, a, sizeof(a), 0, &h) == VM_OK &&
      h.tag == 0x24);
    C(vm_dwarf_abbrev_attr_at(a, sizeof(a), 0, 0, 0, &af) == VM_OK &&
      af.implicit_const == -2);
    die[0] = 3;
    C(vm_dwarf_die_header(die, sizeof(die), 0, a, sizeof(a), 0, &h) == VM_OK &&
      h.tag == 0x11);
    die[0] = 2;
    C(vm_dwarf_die_header(die, sizeof(die), 0, a, sizeof(a), 0, &h) ==
      VM_NOT_FOUND);
    for (i = 0; i < sizeof(a); ++i)
    {
        n = 777;
        C(vm_dwarf_abbrev_count_checked(a, i, 0, &n) == VM_FORMAT && n == 777);
    }
    C(vm_dwarf_abbrev_count_checked(duplicate, sizeof(duplicate), 0, &n) ==
      VM_FORMAT);
    C(vm_dwarf_abbrev_count_checked(half_end, sizeof(half_end), 0, &n) ==
      VM_FORMAT);
    C(vm_dwarf_abbrev_count_checked(
          bad_children, sizeof(bad_children), 0, &n) == VM_FORMAT);
    return 0;
}

static int expressions(void)
{
    unsigned char b[16] = {0};
    vm_dwarf_value_t value = {0};
    uint64_t out;
    int be;
    value.kind = VM_DWARF_BLOCK;
    value.data = b;
    for (be = 0; be < 2; ++be)
    {
        b[0] = 3;
        put(b, 1, UINT64_C(0x123456789abcdef0), 8, be);
        value.size = 9;
        C(vm_dwarf_location_address(&value, 8, be, &out) == VM_OK);
        C(out == UINT64_C(0x123456789abcdef0));
        value.size = 10;
        out = 55;
        C(vm_dwarf_location_address(&value, 8, be, &out) == VM_UNSUPPORTED &&
          out == 55);
    }
    b[0] = 0x23;
    b[1] = 0x80;
    b[2] = 1;
    value.size = 3;
    C(vm_dwarf_member_offset(&value, &out) == VM_OK && out == 128);
    value.size = 2;
    C(vm_dwarf_member_offset(&value, &out) == VM_FORMAT);
    value.size = 4;
    C(vm_dwarf_member_offset(&value, &out) == VM_UNSUPPORTED);
    value.kind = VM_DWARF_SIGNED;
    value.signed_value = -1;
    C(vm_dwarf_member_offset(&value, &out) == VM_UNSUPPORTED);
    value.signed_value = 32;
    C(vm_dwarf_member_offset(&value, &out) == VM_OK && out == 32);
    value.kind = VM_DWARF_SECTION_OFFSET;
    C(vm_dwarf_location_address(&value, 4, 0, &out) == VM_UNSUPPORTED);
    return 0;
}

typedef struct
{
    size_t calls;
    int cancel;
    int mismatch;
} visit_context;
static vm_status_t visit(void *context, const vm_dwarf_die_view_t *die)
{
    visit_context *c = context;
    const vm_dwarf_value_t *name = vm_dwarf_die_attribute(die, VM_DW_AT_name);
    const vm_dwarf_value_t *type = vm_dwarf_die_attribute(die, VM_DW_AT_type);
    c->calls++;
    if (c->cancel)
    {
        return VM_BUSY;
    }
    if (die->depth == 1)
    {
        if (!name || name->kind != VM_DWARF_STRING ||
            strcmp((const char *)name->data, "rpm") || !type ||
            type->kind != VM_DWARF_INFO_REFERENCE || type->unsigned_value != 11)
        {
            c->mismatch = 1;
        }
    }
    return VM_OK;
}
static int walk(void)
{
    unsigned char a[] = {
        3, 0x11, 1, 0, 0, 9, 0x34, 0, 3, 0x0e, 0x49, 0x13, 0, 0, 0};
    unsigned char b[64] = {0};
    unsigned char strings[] = {0, 'r', 'p', 'm', 0};
    vm_dwarf_sections_t s = {
        {b, 22}, {a, sizeof(a)}, {strings, sizeof(strings)}, {NULL, 0}, 0};
    vm_dwarf_error_t error;
    visit_context c = {0};
    put(b, 0, 18, 4, 0);
    put(b, 4, 4, 2, 0);
    b[10] = 4;
    b[11] = 3;
    b[12] = 9;
    put(b, 13, 1, 4, 0);
    put(b, 17, 11, 4, 0);
    b[21] = 0;
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_OK && c.calls == 2 &&
      !c.mismatch);
    c.calls = 0;
    c.cancel = 1;
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_BUSY && c.calls == 1);
    c.cancel = 0;
    b[12] = 7;
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_FORMAT && error.offset == 12);
    b[12] = 9;
    put(b, 13, 99, 4, 0);
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_FORMAT && error.offset == 13);
    put(b, 13, 1, 4, 0);
    strings[4] = 'x';
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_FORMAT);
    strings[4] = 0;
    put(b, 17, 22, 4, 0);
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_FORMAT);
    put(b, 17, 11, 4, 0);
    /* A second CU cannot supply missing bytes to an attribute in the first. */
    put(b, 0, 13, 4, 0);
    s.info.size = sizeof(b);
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_FORMAT && error.offset == 17);
    put(b, 0, 17, 4, 0);
    s.info.size = 21;
    C(vm_dwarf_walk(&s, visit, &c, &error) == VM_FORMAT && error.offset == 21);
    return 0;
}
int main(void)
{
    C(forms() == 0);
    C(abbreviations() == 0);
    C(expressions() == 0);
    C(walk() == 0);
    puts("DWARF form, abbreviation and DIE tests passed.");
    return 0;
}
