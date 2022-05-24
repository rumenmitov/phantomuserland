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

using namespace Phantom;

extern "C"
{

#include <arch/arch_vmem_util.h>
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

        // Check if we are in object space
        // if (!hal_addr_is_in_object_vmem(page_start_addr))
        // {
        //     Genode::error("Trying to map outside obj.space! ", page_start_addr);
        //     return;
        // }

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

    errno_t hal_alloc_vaddress(void **result, int n_pages) // alloc address of a page, but not memory
    {
        // XXX : Probably, can be optimized in future
        // void *temp_res = nullptr;

        // alloc_aligned() is used internally by alloc(). It also allows to set the from and to parameters

        Genode::log("Allocation avail=", main_obj->_vmem_adapter._obj_space_allocator.avail(), " consumed=", main_obj->_vmem_adapter._obj_space_allocator.consumed());

        // Genode::Range_allocator::Range alloc_range = {
        //     main_obj->_vmem_adapter.OBJECT_SPACE_START,
        //     main_obj->_vmem_adapter.OBJECT_SPACE_START + main_obj->_vmem_adapter.OBJECT_SPACE_SIZE};

        // Genode::Allocator::Alloc_result alloc_res = main_obj->_vmem_adapter._obj_space_allocator.alloc_aligned((Genode::size_t)PAGE_SIZE * num, (unsigned)log2(PAGE_SIZE), alloc_range);

        bool is_ok = false;

        // Range_allocator::Range_result
        //     alloc_res =
        main_obj->_vmem_adapter
            ._obj_space_allocator
            .try_alloc(n_pages * PAGE_SIZE)
            .with_result([&](void *addr)
                         {
                        *result = addr;
                        is_ok = true; },
                         [&](Range_allocator::Alloc_error err)
                         {
                             log("Failed to allocate ", n_pages, " pages in obj space! err:", err);
                             is_ok = false;
                         });

        return is_ok ? 0 : 1;
    }

    void hal_free_vaddress(void *addr, int num)
    {
        // num = number of pages
        void *obj_space_addr = (void *)((char *)addr);
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

        // Genode::log("Phys alloc consumed ram : ", main_obj->_vmem_adapter._pseudo_phys_heap.consumed());

        try
        {
            if (npages <= 0)
            {
                return 1;
            }

            void *temp_res = nullptr;
            // temp_res = main_obj->_vmem_adapter._pseudo_phys_heap.alloc(PAGE_SIZE * npages);
            temp_res = main_obj->_vmem_adapter.alloc_pseudo_phys(npages);

            *result = (physaddr_t)temp_res;

            Genode::log("Phys alloc: numpages= : ", npages, " addr=", Hex((physaddr_t)temp_res));

            return 0;
        }
        catch (Out_of_caps)
        {
            Genode::log("Failed to allocate ", npages, " phys pages, Out_of_caps");
        }
        catch (Out_of_ram)
        {
            Genode::log("Failed to allocate ", npages, " phys pages, Out_of_ram");
        }
        catch (Service_denied)
        {
            Genode::log("Failed to allocate ", npages, " pages, Denied");
        }

        return 1; // XXX : Not sure if it is ok
    }

    void hal_free_phys_pages(physaddr_t paddr, int npages)
    {
        // main_obj->_vmem_adapter._pseudo_phys_heap.free((void *)paddr, npages);
        main_obj->_vmem_adapter.free_pseudo_phys((void *)paddr, npages);
    }

    errno_t hal_alloc_phys_page(physaddr_t *result)
    {
        return hal_alloc_phys_pages(result, 1);
    }

    void hal_free_phys_page(physaddr_t paddr) // alloc and not map - WILL PANIC if page is mapped!
    {
        hal_free_phys_pages(paddr, 1);
    }

    // Required by page fault handler. It is always enabled, so return 1
    int arch_is_object_land_access_enabled()
    {
        return 1;
    }

    int genode_register_page_fault_handler(int (*pf_handler)(void *address, int write, int ip, struct trap_state *ts))
    {
        Genode::log("Registering page fault handler (", Hex((addr_t)pf_handler), ")!");
        main_obj->_vmem_adapter.fault_handler.register_fault_handler(pf_handler);
        return 0;
    }
}