#include "vm_dwarf.h"
#include <stdio.h>
#include <string.h>
#define C(x)                                                                   \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            fprintf(stderr, "DWARF FAIL %d: %s\n", __LINE__, #x);              \
            return 1;                                                          \
        }                                                                      \
    } while (0)
static void put(unsigned char *b, size_t o, uint64_t v, unsigned w, int be)
{
    unsigned i;
    for (i = 0; i < w; i++)
    {
        b[o + i] = (unsigned char)(v >> (8 * (be ? w - 1 - i : i)));
    }
}
int main(void)
{
    unsigned char b[64];
    vm_dwarf_unit_header_t h;
    int be;
    int wide;
    int version;
    size_t i;
    size_t p;
    size_t n;
    for (be = 0; be < 2; be++)
    {
        for (wide = 0; wide < 2; wide++)
        {
            for (version = 2; version <= 5; version++)
            {
                memset(b, 0, sizeof(b));
                p = wide ? 12 : 4;
                n = p + (version == 5 ? 4 : 3) + (wide ? 8 : 4);
                if (wide)
                {
                    put(b, 0, UINT32_MAX, 4, be);
                    put(b, 4, n - p + 1, 8, be);
                }
                else
                {
                    put(b, 0, n - p + 1, 4, be);
                }
                put(b, p, version, 2, be);
                if (version == 5)
                {
                    b[p + 2] = 1;
                    b[p + 3] = 4;
                    put(b, p + 4, 123, wide ? 8 : 4, be);
                }
                else
                {
                    put(b, p + 2, 123, wide ? 8 : 4, be);
                    b[n - 1] = 4;
                }
                C(vm_dwarf_probe_unit_ex(b, n + 1, be, &h) == VM_OK);
                C(h.version == version && h.header_size == n &&
                  h.total_size == n + 1 && h.abbrev_offset == 123);
                C(h.dwarf64 == wide && h.address_size == 4);
                for (i = 0; i < n + 1; i++)
                {
                    C(vm_dwarf_probe_unit_ex(b, i, be, &h) == VM_FORMAT);
                }
                /* Adjacent bytes cannot complete a header whose declared unit is too short. */
                if (wide)
                {
                    put(b, 4, 2, 8, be);
                }
                else
                {
                    put(b, 0, 2, 4, be);
                }
                C(vm_dwarf_probe_unit_ex(b, sizeof(b), be, &h) == VM_FORMAT);
            }
        }
    }
    memset(b, 0, sizeof(b));
    put(b, 0, 0xfffffff0u, 4, 0);
    C(vm_dwarf_probe_unit(b, sizeof(b), &h) == VM_FORMAT);
    {
        unsigned char a[] = {1, 0x11, 1, 3, 8, 0, 0, 2, 0x24, 0, 3, 8, 0, 0, 0};
        vm_dwarf_abbrev_view_t v;
        vm_dwarf_attr_form_t af;
        vm_dwarf_die_header_t dh;
        C(vm_dwarf_abbrev_count(a, sizeof(a)) == 2);
        C(vm_dwarf_abbrev_at(a, sizeof(a), 1, &v) == VM_OK);
        C(v.code == 2 && v.tag == 0x24 && v.attributes == 1 &&
          v.has_children == 0);
        C(vm_dwarf_abbrev_attr_at(a, sizeof(a), 0, 0, 0, &af) == VM_OK);
        C(af.attribute == 3 && af.form == 8);
        {
            unsigned char die[] = {2, 0x7f};
            C(vm_dwarf_die_header(die, sizeof(die), 0, a, sizeof(a), 0, &dh) ==
              VM_OK);
            C(dh.abbrev_code == 2 && dh.tag == 0x24 &&
              dh.attributes_offset == 1);
        }
    }
    puts("DWARF header tests passed (v2-v5, DWARF32/64, both byte orders).");
    return 0;
}
