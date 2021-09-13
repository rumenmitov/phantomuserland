/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Intel ia32 page table support.
 * 
 * CONF_DUAL_PAGEMAP:
 * 
 * We keep two page directories. One covers all memory, other excludes 
 * persistent (paged) memory range. Two pagemaps are switched to enable/
 * disable persistent memory access for current thread.
 * 
 * TODO rename all funcs to arch_ prefix
 *
**/

#define DEBUG_MSG_PREFIX "paging"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <malloc.h>
#include <hal.h>
#include <machdep.h>

#include "phantom_env.h"

//#include "genode_misc.h"

/*
*
*  Paging
*
*/

static int paging_inited = 0;

// /// Differentiate between page tables in paged and non-page areas
// int ptep_is_paged_area(int npde)
// {
//     _stub_print();
//     return 1;
// }

void phantom_paging_init(void)
{
    // _stub_print();
    phantom_paging_start();
}

void phantom_paging_start(void)
{
    // _stub_print();
}

#if CONF_DUAL_PAGEMAP
/**
 * \brief Enable or disable paged mem access. Must be called just 
 * from t_set_paged_mem() in threads lib, as it saves cr3 state for 
 * thread switch.
 * 
 * \return cr3 value for threads lib to save.
 * 
**/
// int32_t arch_switch_pdir(bool paged_mem_enable)
// {
//     _stub_print();
//     return 0;
// }

// int32_t arch_get_pdir(bool paged_mem_enable)
// {
//     _stub_print();
//     return 0;
// }

// int arch_is_object_land_access_enabled() //< check if current thread attempts to access object space having access disabled
// {
//     _stub_print();
//     return 0;
// }

#endif

/* 
*
* Virtual memory control
*
*/

// Used to map and unmap pages
void hal_page_control_etc(
    physaddr_t p, void *page_start_addr,
    page_mapped_t mapped, page_access_t access,
    u_int32_t flags)
{
    // Phantom::main_obj.
}

/*
*
*  Physical Allocation
*
*/

void phantom_phys_alloc_init_static(physalloc_t *arena, u_int32_t n_alloc_units, void *mapbuf)
{
    _stub_print();
}

void phantom_phys_alloc_init(physalloc_t *arena, u_int32_t n_alloc_units)
{
    _stub_print();
}

errno_t phantom_phys_alloc_page(physalloc_t *arena, physalloc_item_t *ret)
{
    _stub_print();
}

void phantom_phys_free_page(physalloc_t *arena, physalloc_item_t free)
{
    _stub_print();
}

errno_t phantom_phys_alloc_region(physalloc_t *arena, physalloc_item_t *ret, size_t len)
{
    _stub_print();
}

void phantom_phys_free_region(physalloc_t *arena, physalloc_item_t start, size_t n_pages)
{
    _stub_print();
}
