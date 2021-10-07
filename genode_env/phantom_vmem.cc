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

#include <stddef.h>
#include <stdint.h>

#include "phantom_env.h"
// #include "phantom_vmem.h"

//#include "genode_misc.h"

extern "C"
{

#include <machdep.h>
#include <hal.h>

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

    void phantom_paging_start(void)
    {
        // _stub_print();
    }

    void phantom_paging_init(void)
    {
        // _stub_print();
        phantom_paging_start();
        paging_inited = 1;
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
        (void)flags;

        bool writeable = false;
        if (access == page_readwrite or access == page_rw)
        {
            writeable = true;
        }

        // Phantom::main_obj->_vme
        if (mapped == page_map)
        {
            Phantom::main_obj->_vmem_adapter.map_page(p, (addr_t)page_start_addr, writeable);
        }
        else if (mapped == page_unmap)
        {
            Phantom::main_obj->_vmem_adapter.unmap_page((addr_t)page_start_addr);
        }
        else
        {
            warning("IO mapping is not supported yet");
        }
    }

    /*
    *
    *  Physical Allocation
    *
    */

#if 0 // Disabled since implementing higher level interface (HAL)

void phantom_phys_alloc_init_static(physalloc_t *arena, u_int32_t n_alloc_units, void *mapbuf)
{
    // _stub_print();
}

void phantom_phys_alloc_init(physalloc_t *arena, u_int32_t n_alloc_units)
{
    // _stub_print();
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

#endif

    /*
    *
    * HAL functions to allocate/free phys/virtual memory
    *
    */

    errno_t hal_alloc_vaddress(void **result, int num) // alloc address of a page, but not memory
    {
        // XXX : Probably, can be optimized in future
        void *temp_res = nullptr;

        // alloc_aligned() is used internally by alloc(). It also allows to set the from and to parameters
        bool alloc_ok = main_obj->_vmem_adapter._obj_space_allocator.alloc_aligned(PAGE_SIZE * num, &temp_res,
                                                                                   log2(sizeof(addr_t)),
                                                                                   0,
                                                                                   main_obj->_vmem_adapter.OBJECT_SPACE_SIZE)
                            .ok();

        *result = (void *)((char *)temp_res + main_obj->_vmem_adapter.OBJECT_SPACE_START);

        if (!alloc_ok)
        {
            Genode::error("Failed to allocate %d pages in object space!", num);
            return 1; // XXX : Not sure if it is ok
        }

        return 0;
    }

    void hal_free_vaddress(void *addr, int num)
    {
        // num = number of pages
        void *obj_space_addr = (void *)((char *)addr - main_obj->_vmem_adapter.OBJECT_SPACE_START);
        main_obj->_vmem_adapter._obj_space_allocator.free(obj_space_addr, num);
    }

    void hal_init_physmem_alloc(void)
    {
    }

    void hal_init_physmem_alloc_thread(void)
    {
        // #if USE_RESERVE
        //     hal_start_thread( replentishThread, 0, THREAD_FLAG_KERNEL );
        // #endif
        //     dbg_add_command(&cmd_mem_stat, "mem", "Physical memory stats");
    }

    errno_t hal_alloc_phys_pages(physaddr_t *result, int npages) // alloc and not map
    {
        void *temp_res = nullptr;
        bool alloc_ok = main_obj->_vmem_adapter._pseudo_phys_heap.alloc(PAGE_SIZE * npages, &temp_res);

        *result = (physaddr_t)temp_res;

        if (!alloc_ok)
        {
            Genode::error("Failed to allocate %d pseudo physical pages! (probably, OOM)", npages);
            return 1; // XXX : Not sure if it is ok
        }

        return 0;
    }

    void hal_free_phys_pages(physaddr_t paddr, int npages)
    {
        main_obj->_vmem_adapter._pseudo_phys_heap.free((void *)paddr, npages);
    }

    errno_t hal_alloc_phys_page(physaddr_t *result)
    {
        return hal_alloc_phys_pages(result, 1);
    }

    void hal_free_phys_page(physaddr_t paddr) // alloc and not map - WILL PANIC if page is mapped!
    {
        hal_free_phys_pages(paddr, 1);
    }
}