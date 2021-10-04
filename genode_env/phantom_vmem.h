#ifndef GENODE_PHANTOM_VMEM_H
#define GENODE_PHANTOM_VMEM_H

#include <rm_session/connection.h>
#include <region_map/client.h>
#include <dataspace/client.h>
#include <base/heap.h>
#include <base/entrypoint.h>

#include "phantom_env.h"

#include <arch/arch-page.h>

using namespace Genode;

namespace Phantom
{
    class Local_fault_handler;
    class Vmem_adapter;
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

    volatile unsigned _fault_cnt{0};

    void _handle_fault()
    {
        Region_map::State state = _region_map.state();

        _fault_cnt++;

        log("region-map state is ",
            state.type == Region_map::State::READ_FAULT ? "READ_FAULT" : state.type == Region_map::State::WRITE_FAULT ? "WRITE_FAULT"
                                                                     : state.type == Region_map::State::EXEC_FAULT    ? "EXEC_FAULT"
                                                                                                                      : "READY",
            ", pf_addr=", Hex(state.addr, Hex::PREFIX));

        log("allocate dataspace and attach it to sub region map");
        Dataspace_capability ds = _env.ram().alloc(_page_size);
        _region_map.attach_at(ds, state.addr & ~(_page_size - 1));

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

    void dissolve() { Entrypoint::dissolve(_handler); }

    unsigned fault_count()
    {
        asm volatile("" ::
                         : "memory");
        return _fault_cnt;
    }
};

// class Phantom::Constrained_allocator

struct Phantom::Vmem_adapter
{

    const addr_t OBJECT_SPACE_SIZE = 0x80000000;
    const addr_t OBJECT_SPACE_START = 0x80000000;
    // TODO : defined as a macro that is required for other Phantom, need to fix!!!
    // const size_t PAGE_SIZE = 4096;

    const unsigned int _phys_rm_size = 1024 * 1024 * 512;

    Genode::Env &_env;
    Genode::Rm_connection _rm{_env};

    // Not attached fully, but pages from it supposed to be mapped on object space
    Genode::Region_map_client _pseudo_phys_rm{_rm.create(_phys_rm_size)};
    // Heap (allocator) that allocates dataspace per each allocated page and maps it on _pseudo_phys_rm
    Genode::Sliced_heap _pseudo_phys_heap{_env.ram(), _pseudo_phys_rm};

    // Attached to env's rm
    Genode::Region_map_client _obj_space{_rm.create(OBJECT_SPACE_SIZE)};
    // TODO : modify allocator to fit the size of obj. space. Should be simple, need to
    //        create a derived class and tune alloc() to handle appropriate size
    Genode::Allocator_avl _obj_space_allocator{0};

    Vmem_adapter(Env &env) : _env(env)
    {

        Phantom::Local_fault_handler fault_handler(env, _obj_space, PAGE_SIZE);
        // ATTENTION! _obj_space is attached to the env's rm!
        env.rm().attach(_obj_space.dataspace(), 0, 0, true, OBJECT_SPACE_START, false, true);
    }

    void alloc_phantom_phys_page(void **addr)
    {
        // TODO : Error handling
        _pseudo_phys_heap.alloc(PAGE_SIZE, addr);
    }

    void free_phantom_phys_page(void *addr)
    {
        // TODO : Error handling
        _pseudo_phys_heap.free(addr, PAGE_SIZE);
    }

    void map_page(addr_t phys_addr, addr_t virt_addr, bool writeable)
    {
        if (virt_addr >= OBJECT_SPACE_START && virt_addr < OBJECT_SPACE_START + OBJECT_SPACE_SIZE)
        {
            // TODO : Error handling
            // _obj_sp
            _obj_space.attach(_pseudo_phys_rm.dataspace(), PAGE_SIZE, phys_addr, true, virt_addr - OBJECT_SPACE_START, false, writeable);
            // _obj_space.attach_at(_pseudo_phys_rm.dataspace(), virt_addr - OBJECT_SPACE_START, PAGE_SIZE, phys_addr);
        }
        else
        {
            warning("Trying to map outside object space! Will not map");
        }
    }

    void unmap_page(addr_t virt_addr)
    {
        if (virt_addr >= OBJECT_SPACE_START && virt_addr < OBJECT_SPACE_START + OBJECT_SPACE_SIZE)
        {
            // TODO : Error handling
            _obj_space.detach(virt_addr);
        }
        else
        {
            warning("Trying to map outside object space! Will not map");
        }
    }
};

#endif