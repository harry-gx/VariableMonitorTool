#ifndef VM_DWARF_VALUES_H
#define VM_DWARF_VALUES_H
/* DWARF 2-5 standard encodings used by this reader. */
enum
{
    VM_DW_TAG_subrange_type = 0x21,
    VM_DW_TAG_array_type = 0x01,
    VM_DW_TAG_member = 0x0d,
    VM_DW_TAG_enumeration_type = 0x04,
    VM_DW_TAG_enumerator = 0x28,
    VM_DW_TAG_pointer_type = 0x0f,
    VM_DW_TAG_compile_unit = 0x11,
    VM_DW_TAG_structure_type = 0x13,
    VM_DW_TAG_typedef = 0x16,
    VM_DW_TAG_union_type = 0x17,
    VM_DW_TAG_base_type = 0x24,
    VM_DW_TAG_const_type = 0x26,
    VM_DW_TAG_variable = 0x34,
    VM_DW_TAG_volatile_type = 0x35,
    VM_DW_TAG_partial_unit = 0x3c
};
enum
{
    VM_DW_AT_language = 0x13,
    VM_DW_AT_const_value = 0x1c,
    VM_DW_AT_bit_offset = 0x0c,
    VM_DW_AT_bit_size = 0x0d,
    VM_DW_AT_data_bit_offset = 0x6b,
    VM_DW_AT_lower_bound = 0x22,
    VM_DW_AT_upper_bound = 0x2f,
    VM_DW_AT_count = 0x37,
    VM_DW_AT_bit_stride = 0x2e,
    VM_DW_AT_byte_stride = 0x51,
    VM_DW_AT_location = 0x02,
    VM_DW_AT_name = 0x03,
    VM_DW_AT_byte_size = 0x0b,
    VM_DW_AT_producer = 0x25,
    VM_DW_AT_data_member_location = 0x38,
    VM_DW_AT_type = 0x49
};
enum
{
    VM_DW_FORM_addr = 0x01,
    VM_DW_FORM_block2 = 0x03,
    VM_DW_FORM_block4 = 0x04,
    VM_DW_FORM_data2 = 0x05,
    VM_DW_FORM_data4 = 0x06,
    VM_DW_FORM_data8 = 0x07,
    VM_DW_FORM_string = 0x08,
    VM_DW_FORM_block = 0x09,
    VM_DW_FORM_block1 = 0x0a,
    VM_DW_FORM_data1 = 0x0b,
    VM_DW_FORM_flag = 0x0c,
    VM_DW_FORM_sdata = 0x0d,
    VM_DW_FORM_strp = 0x0e,
    VM_DW_FORM_udata = 0x0f,
    VM_DW_FORM_ref_addr = 0x10,
    VM_DW_FORM_ref1 = 0x11,
    VM_DW_FORM_ref2 = 0x12,
    VM_DW_FORM_ref4 = 0x13,
    VM_DW_FORM_ref8 = 0x14,
    VM_DW_FORM_ref_udata = 0x15,
    VM_DW_FORM_indirect = 0x16,
    VM_DW_FORM_sec_offset = 0x17,
    VM_DW_FORM_exprloc = 0x18,
    VM_DW_FORM_flag_present = 0x19,
    VM_DW_FORM_data16 = 0x1e,
    VM_DW_FORM_line_strp = 0x1f,
    VM_DW_FORM_ref_sig8 = 0x20,
    VM_DW_FORM_implicit_const = 0x21
};
#endif
