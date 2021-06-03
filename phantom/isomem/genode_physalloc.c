#include <kernel/physalloc.h>
#include <stddef.h>
#include <stdint.h>
#include "genode_misc.h"

static hal_mutex_t *heap_lock = 0; // won't be used until assigned

void 	phantom_phys_alloc_init_static(physalloc_t *arena, u_int32_t n_alloc_units, void *mapbuf){
  _stub_print();
}

void 	phantom_phys_alloc_init(physalloc_t *arena, u_int32_t n_alloc_units ){
  _stub_print();
}

errno_t phantom_phys_alloc_page( physalloc_t *arena, physalloc_item_t *ret ){
  _stub_print();
}

void 	phantom_phys_free_page( physalloc_t *arena, physalloc_item_t free ){
  _stub_print();
}

errno_t phantom_phys_alloc_region( physalloc_t *arena, physalloc_item_t *ret, size_t len ){
  _stub_print();
}

void 	phantom_phys_free_region( physalloc_t *arena, physalloc_item_t start, size_t n_pages ){
  _stub_print();
}

// Called after initing threads (and mutexes)
void heap_init_mutex(void)
{
    // Mutex init uses malloc! First allocate it (malloc will run unprotected),
    // then assign, turning on protection

    static hal_mutex_t mutex;
    if(hal_mutex_init(&mutex,"Heap") < 0) {
        panic("error creating heap mutex\n");
    }

    heap_lock = &mutex;
}