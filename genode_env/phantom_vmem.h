#ifndef GENODE_PHANTOM_VMEM_H
#define GENODE_PHANTOM_VMEM_H

#include <rm_session/connection.h>
#include <region_map/client.h>
#include <dataspace/client.h>
#include <base/heap.h>
#include <base/entrypoint.h>
#include <base/registry.h>

#include <cpu/memory_barrier.h>

#include "phantom_env.h"
#include "local_attached_ram_dataspace.h"

#include <arch/arch-page.h>

#include <arch/arch_vmem_trap_state.h>

using namespace Genode;

namespace Phantom
{
    class Local_fault_handler;
    class Vmem_adapter;
    class Aligned_page_allocator;
};

/**
 * Region-manager fault handler resolves faults by attaching new dataspaces
 */
class Phantom::Local_fault_handler : public Genode::Entrypoint
{
private:
    Genode::Env &_env;
    Region_map &_region_map;
    Signal_handler<Local_fault_handler> _handler;
    size_t _page_size;

    // Handler set by Phantom init routine
    int (*_pf_handler)(void *address, int write, int ip, struct trap_state *ts) = nullptr;

    volatile unsigned _fault_cnt{0};

    int test_pf_handler(void *address, int write, int ip, struct trap_state *ts)
    {
        // TODO : Need to clean up after this thing

        warning("Test page fault handler! addr=", Hex((addr_t)address), " write=", write, " ip=", Hex(ip), " ts=", Hex((addr_t)ts));
        // log("PF: addr=%p, ip=%p, write=%d, ts.state=%d", address, write, ip, ts->state);
        Ram_dataspace_capability ds = _env.ram().alloc(_page_size);
        // _region_map.attach_at(ds, (Genode::addr_t)address & ~(_page_size - 1));
        return 0;
    }

    void _handle_fault()
    {
        log("Reached fault handler");

        Region_map::State state = _region_map.state();

        _fault_cnt++;

        log("region-map state is ",
            state.type == Region_map::State::READ_FAULT ? "READ_FAULT" : state.type == Region_map::State::WRITE_FAULT ? "WRITE_FAULT"
                                                                     : state.type == Region_map::State::EXEC_FAULT    ? "EXEC_FAULT"
                                                                                                                      : "READY",
            ", pf_addr=", Hex(state.addr, Hex::PREFIX));

        struct trap_state ts_stub;
        ts_stub.state = 0;

        // Calling external handler
        // TODO : fix IP
        if (_pf_handler != nullptr)
        {
            _pf_handler((void *)state.addr, state.type == state.WRITE_FAULT ? 1 : 0, -1, &ts_stub);
        }
        else
        {
            test_pf_handler((void *)state.addr, state.type == state.WRITE_FAULT ? 1 : 0, -1, &ts_stub);
        }

        log("returning from handle_fault");
    }

public:
    Local_fault_handler(Genode::Env &env, Region_map &region_map, size_t page_size)
        : Entrypoint(env, sizeof(addr_t) * 2048, "local_fault_handler",
                     Affinity::Location()),
          _env(env),
          _region_map(region_map),
          _handler(*this, *this, &Local_fault_handler::_handle_fault),
          _page_size(page_size)
    {
        region_map.fault_handler(_handler);

        log("fault handler: waiting for fault signal");
    }

    // Have to intialize it dynamically since Phantom registers page fault handler after intialization of object space
    void register_fault_handler(int (*pf_handler)(void *address, int write, int ip, struct trap_state *ts))
    {
        _pf_handler = pf_handler;
    }

    void dissolve() { Entrypoint::dissolve(_handler); }

    unsigned fault_count()
    {
        asm volatile("" ::
                         : "memory");
        return _fault_cnt;
    }
};

// Handle to put inside registry (used inside vmem adapter)
typedef Registered<Local_attached_ram_dataspace>
    Attached_ds_handle;

struct Phantom::Vmem_adapter
{

    const addr_t OBJECT_SPACE_START = 0x80000000;
    const addr_t OBJECT_SPACE_SIZE = 0x40000000;
    // TODO : defined as a macro that is required for other Phantom, need to fix!!!
    // const size_t PAGE_SIZE = 4096;

    const unsigned int _phys_rm_size = 0x40000000;

    Genode::Env &_env;
    Genode::Rm_connection _rm{_env};

    Genode::Heap _metadata_heap{_env.ram(), _env.rm()};

    // Attached to env's rm
    Genode::Region_map_client _obj_space{_rm.create(OBJECT_SPACE_SIZE)};
    // fault handler (will register handler in rm as well)
    Phantom::Local_fault_handler fault_handler{_env, _obj_space, PAGE_SIZE};

    // Not attached fully, but pages from it supposed to be mapped on object space
    // Genode::Region_map_client _pseudo_phys_rm{_rm.create(_phys_rm_size)};
    Genode::Region_map_client _pseudo_phys_rm{_rm.create(_phys_rm_size)};
    // Heap (allocator) that allocates dataspace per each allocated page and maps it on _pseudo_phys_rm
    // Genode::Heap _pseudo_phys_heap{_env.ram(), _env.rm()};

    // Registry to keep track of attached dataspaces to the pseudo phys rm
    Genode::Registry<Attached_ds_handle> _pseudo_phys_ds_registry{};

    // TODO : modify allocator to fit the size of obj. space. Should be simple, need to
    //        create a derived class and tune alloc() to handle appropriate size
    Genode::Allocator_avl _obj_space_allocator{&_metadata_heap};

    // Attached_ram_dataspace _zero_pseudo_phys_page{_env.ram(), _pseudo_phys_rm, PAGE_SIZE * 2};

    Vmem_adapter(Env &env) : _env(env)
    {

        // Initializing obj space allocator
        _obj_space_allocator.add_range(OBJECT_SPACE_START + PAGE_SIZE * 0x10, OBJECT_SPACE_SIZE);

        // ATTENTION! _obj_space is attached to the env's rm!void         *ptr_top  = env.rm().attach(rm_top.dataspace());
        // void *ptr_obj = env.rm().attach(_obj_space.dataspace(), 0, 0, true, OBJECT_SPACE_START, false, true);
        void *ptr_obj = env.rm().attach_at(_obj_space.dataspace(), OBJECT_SPACE_START, OBJECT_SPACE_SIZE);

        Dataspace_client rm_obj_client(_obj_space.dataspace());
        addr_t const addr_obj = reinterpret_cast<addr_t>(ptr_obj);
        log(" region obj.space        ",
            Hex_range<addr_t>(addr_obj, rm_obj_client.size()));

        // Dataspace_client pseudo_phys_ds(_pseudo_phys_rm.dataspace());

        // Phantom expects physical addresses > 0x0. Let's allocate a single page
        // XXX : allocating it bypassing the registry!

        // if (_zero_pseudo_phys_page.local_addr<addr_t>() != 0)
        // {
        //     Genode::error("Failed to allocate pseudo phys page at zero address!");
        // }

        // void *zero_addr = alloc_pseudo_phys(1);

        // if (zero_addr != 0)
        // {
        //     Genode::error("Failed to allocate pseudo phys page at zero address!");
        // }

        // map_page(0, OBJECT_SPACE_START, false);

        // if ((addr_t)alloc_pseudo_phys(1) != 0)
        // {
        //     Genode::error("Failed to allocate pseudo phys page at zero address!");
        // }
    }

    ~Vmem_adapter() {}

    // void alloc_phantom_phys_page(void **addr)
    // {
    //     // TODO : Error handling
    //     try
    //     {
    //         *addr = _pseudo_phys_heap.alloc(PAGE_SIZE);
    //     }
    //     catch (Out_of_caps)
    //     {
    //         Genode::log("Failed to allocate ", npages, " phys pages, Out_of_caps");
    //     }
    //     catch (Out_of_ram)
    //     {
    //         Genode::log("Failed to allocate ", npages, " phys pages, Out_of_ram");
    //     }
    //     catch (Service_denied)
    //     {
    //         Genode::log("Failed to allocate ", npages, " phys pages, Denied");
    //     }
    // }

    // void free_phantom_phys_page(void *addr)
    // {
    //     // TODO : Error handling
    //     _pseudo_phys_heap.free(addr, PAGE_SIZE);
    // }

    void map_page(addr_t phys_addr, addr_t virt_addr, bool writeable)
    {
        (void)writeable;

        log("Mapping phys ", Hex(phys_addr), " to virt ", Hex(virt_addr));

        if (virt_addr >= OBJECT_SPACE_START && virt_addr < OBJECT_SPACE_START + OBJECT_SPACE_SIZE)
        {
            Genode::memory_barrier();

            // TODO : Error handling
            //        Handle 0 return address
            // _obj_sp
            // _obj_space.attach(_pseudo_phys_rm.dataspace(), PAGE_SIZE, phys_addr, true, virt_addr - OBJECT_SPACE_START, false, writeable);

            Region_map::Local_addr laddr = _obj_space.attach(_pseudo_phys_rm.dataspace(), PAGE_SIZE, phys_addr, true, virt_addr - OBJECT_SPACE_START, false, writeable);
            log("Map returned laddr=", Hex((addr_t)laddr));

            // _obj_space.attach_at(_pseudo_phys_rm.dataspace(), virt_addr - OBJECT_SPACE_START, PAGE_SIZE, phys_addr);

            // _pseudo_phys_rm.attach(_obj_space.dataspace(), PAGE_SIZE, phys_addr);
            // _obj_space.attach_at(_pseudo_phys_rm.dataspace(), virt_addr - OBJECT_SPACE_START, PAGE_SIZE, phys_addr);
        }
        else
        {
            warning("Trying to map outside object space! Will not map");
        }
    }

    void unmap_page(addr_t virt_addr)
    {

        log("unmapping virt ", Hex(virt_addr), " obj_space_addr=", virt_addr - OBJECT_SPACE_START);

        if (virt_addr >= OBJECT_SPACE_START && virt_addr < OBJECT_SPACE_START + OBJECT_SPACE_SIZE)
        {

            Genode::memory_barrier();

            // TODO : Error handling
            _obj_space.detach(virt_addr - OBJECT_SPACE_START);
        }
        else
        {
            warning("Trying to map outside object space! Will not map");
        }
    }

    void *alloc_pseudo_phys(unsigned int num_pages)
    {
        // XXX : this implementation is not performant and should be replaced in the future
        // TODO : Add error handling

        // Attached_ram_dataspace is RAI, so all allocations will be handled by it
        Local_attached_ram_dataspace *ds = new (&_metadata_heap) Attached_ds_handle(
            _pseudo_phys_ds_registry,
            _env.ram(),
            _pseudo_phys_rm,
            (size_t)(PAGE_SIZE * num_pages),
            Genode::Cache::CACHED);
        return ds->local_addr<void *>();
    }

    void free_pseudo_phys(void *addr, int npages)
    {
        bool success = false;

        _pseudo_phys_ds_registry.for_each(
            [&](Attached_ds_handle &ds)
            {
                if (ds.local_addr<void *>() != addr)
                {
                    return;
                }

                if (ds.size() != (size_t)(npages * PAGE_SIZE))
                {
                    Genode::warning("free pseudo phys with incorrect number of pages: addr=", Hex((long)addr), " expected=", ds.size(), " received=", npages * PAGE_SIZE);
                    return;
                }

                destroy(_metadata_heap, &ds);
                success = true;
            });

        if (!success)
        {
            Genode::warning("Failed to free pseudo phys: addr=", Hex((long)addr), " npages=", npages);
        }
    }
};

#endif