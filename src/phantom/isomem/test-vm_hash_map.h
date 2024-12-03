#include <phantom_libc.h>
#include <ph_malloc.h>

#include "vm_hash_map.h"


void vm_hash_map_init() {
  u_int64_t mapsize = __N_PAGES * sizeof(vm_page*);
  __vm_hash_map = (vm_page**) ph_malloc(mapsize);

  __vm_map_index = (int*) ph_malloc(__N_PAGES * sizeof(int));
  ph_memset(__vm_map_index, 0, __N_PAGES * sizeof(int));

  // Why 8 tho?
  int bufsize = 8 * sizeof(vm_page);

   vm_page** ar = __vm_hash_map;

    for (size_t i = 0; i < __N_PAGES; i++)
    {
        ar[i] = (vm_page*) ph_malloc(bufsize);
        ph_memset(ar[i], 0, bufsize);
    }
    hal_mutex_init(&__vm_map_mutex, "VM Map");
}


vm_page* vm_hash_page_init(void *v_addr) {
    vm_page* p = (vm_page*) ph_malloc(sizeof(vm_page));
    ph_memset(p, 0, sizeof(vm_page));
    p->virt_addr = v_addr;
    p->exists = 1;

    hal_mutex_init(&p->lock, "VM Page");
    return p;
}


// returns a hash
u_int64_t hash(u_int64_t val) {
    return val % __N_PAGES;
}


vm_page* find_page(void* v_addr) {
  int page_idx = hash((u_int64_t)v_addr);
  // printf("address: %d\n", page_idx);

  vm_page* buf = __vm_hash_map[page_idx];

  int i;
  for (i = 0; i < 8; i++) {
    if (buf[i].exists && buf[i].virt_addr == v_addr)
      {
	return &buf[i];
      }
  }
  // if the page is not found, return the index of the next page to be replace on this hash value
  return buf + __vm_map_index[page_idx];
}


void vm_hash_set_page(vm_page *new_page) {
  void *new_page_vaddr = new_page->virt_addr;
  vm_page *page = find_page(new_page_vaddr);

  hal_mutex_lock(&__vm_map_mutex);
  if (page->exists) {
    hal_mutex_lock(&page->lock);
  } else {
    int page_idx = hash( (u_int64_t)new_page_vaddr );
    __vm_map_index[page_idx] = (__vm_map_index[page_idx] + 1) % 8;
  }

  // TODO add saving state of old page
  // TODO perform pageout

  *page = *new_page;
  hal_mutex_unlock(&page->lock);
  hal_mutex_unlock(&__vm_map_mutex);
}


vm_page vm_hash_get_page(void *v_addr) {
  vm_page *page = find_page(v_addr);
  if (page->virt_addr != v_addr) return (const struct vm_page){0};

  hal_mutex_lock(&page->lock);
  vm_page res = *page;
  hal_mutex_unlock(&page->lock);
  return res;
}





void test1() {
  ph_printf("page init...\n");
  vm_page* p = vm_hash_page_init((void*)1);
  p->phys_addr = 123;

  ph_printf("adding page...\n");
  vm_hash_set_page(p);

  ph_printf("getting page...\n");
  vm_page p2 = vm_hash_get_page((void*)1);

  assert(p->phys_addr == p2.phys_addr);
  assert((void*)&p != (void*)&p2);
  ph_printf("page init test is complete!\n\n");
}

void test2() {
    ph_printf("checking empty page...\n");
    vm_page p3 = vm_hash_get_page((void*)123);
    assert(p3.exists == 0);
    assert(p3.virt_addr == 0);
    assert(p3.phys_addr == 0);
    ph_printf("empty page test is completed\n\n");
}

void test3(int total) {
    ph_printf("counting hits/total rate...\n");
    int hits = 0;
    u_int64_t v_addr;
    for (v_addr = 0; v_addr < total; v_addr++) {
        vm_page* p = vm_hash_page_init((void*)v_addr);
        p->phys_addr = v_addr ^ 123;
        vm_hash_set_page(p);
    }
    for (v_addr = 0; v_addr < total; v_addr++) {
        vm_page p = vm_hash_get_page((void*)v_addr);
        if (p.exists == 1 && (u_int64_t)p.virt_addr == v_addr) {
            assert(p.phys_addr == (v_addr ^ 123));
            hits++;
        }
    }
    ph_printf("hits/total %d/%d\n\n", hits, total);
}

