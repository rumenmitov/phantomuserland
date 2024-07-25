#ifndef __VM_HASH_MAP_H
#define __VM_HASH_MAP_H

#include <kernel/mutex.h>
#include "vm_map.h"

#define __PAGE_SIZE 0x1000
//#define __N_PAGES   0x1000000000
#define __N_PAGES   0x10000

static vm_page **__vm_hash_map;
static int *__vm_map_index;
static hal_mutex_t __vm_map_mutex;


void vm_hash_map_init();
vm_page* vm_hash_page_init(void *v_addr);
void vm_hash_set_page(vm_page* new_page);
vm_page vm_hash_get_page(void *v_addr);

int vm_hash_map_test() {
  return 69;
}

#endif // __VM_HASH_MAP_H
