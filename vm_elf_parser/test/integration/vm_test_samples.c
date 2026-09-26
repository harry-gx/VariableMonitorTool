#include "vm_elf.h"
#include "vm_dwarf.h"
#include "vm_dwarf_values.h"
#include "vm_type_graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"sample failure line %d: %s\n",__LINE__,#x);return 1;}}while(0)
typedef struct {
 size_t dies, variables, names, types, addresses, members, member_offsets;
 int big_endian, version, expected_variable, expected_member, mismatch;
} stats_t;
static vm_status_t visit(void *context,const vm_dwarf_die_view_t *die) {
 stats_t *stats=context;
 const vm_dwarf_value_t *name=vm_dwarf_die_attribute(die,VM_DW_AT_name);
 const vm_dwarf_value_t *type=vm_dwarf_die_attribute(die,VM_DW_AT_type);
 const vm_dwarf_value_t *loc=vm_dwarf_die_attribute(die,VM_DW_AT_location);
 const vm_dwarf_value_t *member=vm_dwarf_die_attribute(die,VM_DW_AT_data_member_location);
 uint64_t address;
 stats->dies++;
 if(die->tag==VM_DW_TAG_variable) {
  stats->variables++;
  if(name && name->kind==VM_DWARF_STRING)stats->names++;
  if(type && type->kind==VM_DWARF_INFO_REFERENCE)stats->types++;
  if(loc && vm_dwarf_location_address(loc,die->address_size,stats->big_endian,&address)==VM_OK) {
   stats->addresses++;
   /* Independent baselines from GNU readelf for the checked-in sample files. */
   if(stats->version==4 && die->offset==0xe43) {
    stats->expected_variable=1;
    if(!name || strcmp((const char*)name->data,"flashSSDConfig") ||
       !type || type->unsigned_value!=0xd74 || address!=0x20000000)stats->mismatch=1;
   }
   if(stats->version==3 && name && name->kind==VM_DWARF_STRING &&
      strcmp((const char*)name->data,"SystemCoreClock")==0) {
    stats->expected_variable=1;
    if(!type || type->kind!=VM_DWARF_INFO_REFERENCE)stats->mismatch=1;
   }
  }
 }
 if(die->tag==VM_DW_TAG_member) {
  stats->members++;
  if(member && vm_dwarf_member_offset(member,&address)==VM_OK) {
   stats->member_offsets++;
   if(stats->version==3 && die->offset==0xe43) {
    stats->expected_member=1;
    if(!name || strcmp((const char*)name->data,"DHR12RD") ||
       !type || type->unsigned_value!=0xa89 || address!=32)stats->mismatch=1;
   }
  }
 }
 return VM_OK;
}
int main(int argc,char **argv) {
 vm_elf_t *elf=NULL;vm_parse_error_t err;vm_elf_section_view_t info,abbrev;
 vm_dwarf_unit_header_t h;size_t offset=0,count=0,versions[6]={0},abbrev_count=0,table_count;
 vm_dwarf_sections_t sections={0};vm_dwarf_error_t dw_error;vm_status_t status;
 vm_elf_section_view_t strings;stats_t stats={0};
 vm_type_graph_t *graph=NULL;vm_type_node_view_t node;
 CHECK(argc==3);
 stats.version=atoi(argv[2]);
 CHECK(vm_elf_parse_file(argv[1],&elf,&err)==VM_OK);
 CHECK(vm_elf_symbol_count(elf)>0);CHECK(vm_elf_has_debug_info(elf));
 CHECK(vm_elf_section(elf,".debug_info",&info)==VM_OK);
 CHECK(vm_elf_section(elf,".debug_abbrev",&abbrev)==VM_OK);
 while(offset<info.size) {
  CHECK(vm_dwarf_probe_unit_ex(info.data+offset,info.size-offset,info.big_endian,&h)==VM_OK);
  CHECK(h.total_size>0 && h.abbrev_offset<abbrev.size);
  CHECK(vm_dwarf_abbrev_count_checked(abbrev.data,abbrev.size,(size_t)h.abbrev_offset,&table_count)==VM_OK);
  CHECK(table_count>0); abbrev_count+=table_count;
  versions[h.version]++;
  offset+=h.total_size;count++;
 }
 CHECK(offset==info.size && count>0);
 CHECK(atoi(argv[2])>=2 && atoi(argv[2])<=5 && versions[atoi(argv[2])]>0);
 printf("%s: symbols=%zu units=%zu abbrev_bytes=%zu debug_info=%zu\n",
        argv[1],vm_elf_symbol_count(elf),count,abbrev.size,info.size);
 printf("units by version: v2=%zu v3=%zu v4=%zu v5=%zu\n",
        versions[2],versions[3],versions[4],versions[5]);
 sections.info.data=info.data;sections.info.size=info.size;
 sections.abbrev.data=abbrev.data;sections.abbrev.size=abbrev.size;
 sections.big_endian=info.big_endian;stats.big_endian=info.big_endian;
 status=vm_elf_section(elf,".debug_str",&strings);
 CHECK(status==VM_OK || status==VM_NOT_FOUND);
 if(status==VM_OK){sections.strings.data=strings.data;sections.strings.size=strings.size;}
 status=vm_dwarf_walk(&sections,visit,&stats,&dw_error);
 if(status!=VM_OK)fprintf(stderr,"DIE error %d: %s + 0x%zx form=0x%" PRIx64 " %s\n",
                         status,dw_error.section,dw_error.offset,dw_error.form,dw_error.message);
 CHECK(status==VM_OK);
 CHECK(stats.dies>count && stats.variables>0 && stats.names>0 && stats.types>0 && stats.addresses>0);
 CHECK(stats.expected_variable && !stats.mismatch);
 CHECK(stats.version!=3 || stats.member_offsets>0);
 CHECK(vm_type_graph_build(&sections,&graph,&dw_error)==VM_OK);
 CHECK(vm_type_graph_count(graph)>stats.variables);
 CHECK(vm_type_graph_variable_count(graph)==stats.variables);
 CHECK(vm_type_graph_static_address_count(graph)==stats.addresses);
 if (stats.version==3) {
  uint64_t system_core_clock_address;
  CHECK(vm_type_graph_find_variable(graph,"SystemCoreClock",&node)==VM_OK);
  CHECK(node.has_location_address);
  system_core_clock_address=node.location_address;
  CHECK(vm_type_graph_find_variable_by_address(graph,system_core_clock_address,&node)==VM_OK);
  CHECK(vm_type_graph_find_variable_containing(graph,system_core_clock_address+1,&node)==VM_OK);
 } else {
  CHECK(vm_type_graph_find_variable(graph,"flashSSDConfig",&node)==VM_OK);
  CHECK(node.has_location_address && node.location_address==0x20000000);
  CHECK(vm_type_graph_find_variable_by_address(graph,0x20000000,&node)==VM_OK);
  CHECK(vm_type_graph_find_variable_containing(graph,0x20000001,&node)==VM_OK);
 }
 CHECK(vm_type_graph_variable_at(graph,0,&node)==VM_OK && node.kind==VM_TYPE_VARIABLE);
 CHECK(node.type_die_offset!=0);
 /* Snapshot must survive release of all borrowed ELF section buffers. */
 vm_elf_close(elf);elf=NULL;
 {
  size_t i,j,member_nodes=0;
  for(i=0;i<vm_type_graph_count(graph);++i) {
   vm_type_node_view_t member,parent,listed;int found=0;
   CHECK(vm_type_graph_at(graph,i,&member)==VM_OK);
   if(member.kind!=VM_TYPE_MEMBER)continue;
   member_nodes++;
   CHECK(vm_type_graph_find_die(graph,member.parent_die_offset,&parent)==VM_OK);
   CHECK(parent.kind==VM_TYPE_STRUCT || parent.kind==VM_TYPE_UNION);
   for(j=0;j<vm_type_graph_member_count(graph,parent.die_offset);++j) {
    CHECK(vm_type_graph_member_at(graph,parent.die_offset,j,&listed)==VM_OK);
    if(listed.die_offset==member.die_offset)found=1;
   }
   CHECK(found);
  }
  CHECK(member_nodes==stats.members);
 }
 {
  vm_type_resolution_t resolved;uint64_t size;
  if(stats.version==3) {
   CHECK(vm_type_graph_find_variable(graph,"SystemCoreClock",&node)==VM_OK);
   CHECK(vm_type_graph_resolve(graph,node.die_offset,&resolved)==VM_OK);
   CHECK(resolved.type.kind==VM_TYPE_BASE);
   CHECK(vm_type_graph_sizeof(graph,node.die_offset,&size)==VM_OK && size==4);
  } else {
   CHECK(vm_type_graph_find_die(graph,0xe43,&node)==VM_OK);
   CHECK(!strcmp(node.name,"flashSSDConfig"));
   CHECK(vm_type_graph_resolve(graph,0xe43,&resolved)==VM_OK);
   CHECK(resolved.type.kind==VM_TYPE_STRUCT && resolved.type.die_offset==0xd0f);
   CHECK(vm_type_graph_sizeof(graph,0xe43,&size)==VM_OK && size==28);
  }
  printf("resolved real-sample type DIE=0x%" PRIx64 " size=%" PRIu64 "\n",
         resolved.type.die_offset,size);
 }
 if(stats.version!=3) {
  uint64_t array_die=0x96, elements, bytes;
  CHECK(vm_type_graph_dimension_count(graph,array_die)==1);
  CHECK(vm_type_graph_array_count(graph,array_die,&elements)==VM_OK);
  CHECK(elements==8);
  CHECK(vm_type_graph_sizeof(graph,array_die,&bytes)==VM_OK);
  CHECK(bytes==8);
  printf("real-sample array elements=%" PRIu64 " bytes=%" PRIu64 "\n",elements,bytes);
 }
 vm_type_graph_destroy(graph);
 printf("DIEs=%zu variables=%zu named=%zu typed=%zu absolute_locations=%zu members=%zu\n",
        stats.dies,stats.variables,stats.names,stats.types,stats.addresses,stats.members);
 printf("resolved member offsets=%zu\n",stats.member_offsets);
 vm_elf_close(elf);return 0;
}
