#include "vm_elf.h"
#include "vm_reader.h"
#include "vm_router.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static void put(uint8_t *b,size_t o,uint32_t v,unsigned w){unsigned i;for(i=0;i<w;i++)b[o+i]=(uint8_t)(v>>(8*i));}
static void fixture(uint8_t *b){
 memset(b,0,256);memcpy(b,"\177ELF",4);b[4]=1;b[5]=1;b[6]=1;
 put(b,16,2,2);put(b,18,40,2);put(b,20,1,4);put(b,32,52,4);put(b,40,52,2);
 put(b,46,40,2);put(b,48,4,2);
 /* null, NOBITS target, STRTAB, SYMTAB */
 put(b,96,8,4);put(b,112,4,4);
 put(b,100,3,4); /* SHF_ALLOC | SHF_WRITE */
 put(b,136,3,4);put(b,148,212,4);put(b,152,5,4);memcpy(b+212,"\0rpm",5);
 put(b,176,2,4);put(b,188,224,4);put(b,192,32,4);put(b,196,2,4);put(b,208,16,4);
 put(b,240,1,4);put(b,244,0x20000000,4);put(b,248,4,4);b[252]=17;put(b,254,1,2);
}
static vm_router_t *router;static int rx,tx;
static vm_status_t receive(void *c,const vm_frame_t *f) {
 CHECK(c==&rx);CHECK(f->address==0x708);rx++;
 CHECK(vm_router_unbind(router,1)==VM_BUSY);return VM_OK;
}
static vm_status_t send_frame(void *c,const vm_frame_t *f) {
 CHECK(c==&tx);CHECK(f->address==0x700);CHECK(f->connection==9);tx++;return VM_IO;
}
int main(void) {
 uint8_t b[256];vm_elf_t *e=NULL;vm_parse_error_t error;vm_symbol_t sym;vm_variable_view_t variable;
 vm_registry_t *reg;const void *ops;vm_adapter_info_t info;size_t i;
 vm_reader_t reader={b,sizeof(b),0};uint64_t value;
 vm_route_t route={1,9,2,0x708,0x700,receive,send_frame,&rx,&tx};
 vm_frame_t frame={9,2,0x708,b,1};
 fixture(b);
 CHECK(vm_read_uint(&reader,SIZE_MAX,4,&value)==VM_FORMAT);
 CHECK(vm_read_uint(&reader,244,4,&value)==VM_OK && value==0x20000000);
 CHECK(vm_registry_create(&reg)==VM_OK);CHECK(vm_parser_register_builtins(reg)==VM_OK);
 CHECK(vm_registry_count(reg)==2);CHECK(vm_registry_at(reg,1,&info)==VM_OK);
 CHECK(strcmp(info.id,"keil_armcc_axf")==0);
 CHECK(vm_parser_register_builtins(reg)==VM_DUPLICATE);
 CHECK(vm_registry_acquire(reg,info.id,&ops)==VM_OK);
 CHECK(vm_registry_remove(reg,info.id)==VM_BUSY);CHECK(vm_registry_destroy(reg)==VM_BUSY);
 CHECK(((const vm_parser_ops_t*)ops)->parse(b,sizeof(b),&e,&error)==VM_OK);
 CHECK(vm_elf_symbol_count(e)==1);CHECK(vm_elf_symbol_at(e,0,&sym)==VM_OK);
 CHECK(!strcmp(sym.name,"rpm") && sym.address==0x20000000 && sym.size==4);
 CHECK(vm_elf_variable_count(e)==1);CHECK(vm_elf_find_variable(e,"rpm",&variable)==VM_OK);
 CHECK(variable.writable && variable.readable && variable.address==sym.address);
 CHECK(vm_elf_find_variable(e,"missing",&variable)==VM_NOT_FOUND);
 b[213]='x';CHECK(!strcmp(sym.name,"rpm"));vm_elf_close(e);fixture(b);
 put(b,100,2,4);
 CHECK(vm_elf_parse(b,sizeof(b),&e,&error)==VM_OK);
 CHECK(vm_elf_variable_at(e,0,&variable)==VM_OK);
 CHECK(variable.readable && !variable.writable);
 vm_elf_close(e);fixture(b);
 put(b,100,0,4);
 CHECK(vm_elf_parse(b,sizeof(b),&e,&error)==VM_OK);
 CHECK(vm_elf_variable_at(e,0,&variable)==VM_OK);
 CHECK(!variable.readable && !variable.writable);
 vm_elf_close(e);fixture(b);
 CHECK(vm_registry_release(reg,info.id)==VM_OK);CHECK(vm_registry_destroy(reg)==VM_OK);
 for(i=0;i<sizeof(b);i++) { CHECK(vm_elf_parse(b,i,&e,&error)!=VM_OK);CHECK(e==NULL); }
 b[4]=2;CHECK(vm_elf_parse(b,sizeof(b),&e,&error)==VM_UNSUPPORTED);fixture(b);
 put(b,32,0xfffffff0,4);CHECK(vm_elf_parse(b,sizeof(b),&e,&error)==VM_FORMAT);fixture(b);
 b[216]='x';CHECK(vm_elf_parse(b,sizeof(b),&e,&error)==VM_FORMAT);fixture(b);
 put(b,196,99,4);CHECK(vm_elf_parse(b,sizeof(b),&e,&error)==VM_FORMAT);
 CHECK(vm_router_create(&router)==VM_OK);CHECK(vm_router_bind(router,&route)==VM_OK);
 CHECK(vm_router_route_count(router)==1);{vm_route_t listed;CHECK(vm_router_route_at(router,0,&listed)==VM_OK && listed.route_id==1);CHECK(vm_router_route_at(router,1,&listed)==VM_NOT_FOUND);}
 CHECK(vm_router_bind(router,&route)==VM_DUPLICATE);route.route_id=2;
 CHECK(vm_router_bind(router,&route)==VM_AMBIGUOUS);
 CHECK(vm_router_receive(router,&frame)==VM_OK && rx==1);
 frame.connection=10;CHECK(vm_router_receive(router,&frame)==VM_NOT_FOUND);
 CHECK(vm_router_send(router,1,b,1)==VM_IO && tx==1);
 CHECK(vm_router_send(router,99,b,1)==VM_NOT_FOUND);
 CHECK(vm_router_unbind(router,1)==VM_OK);CHECK(vm_router_destroy(router)==VM_OK);
 puts("All contract tests passed.");return 0;
}
