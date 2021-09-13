#ifndef GENODE_PHANTOM_VMEM_H
#define GENODE_PHANTOM_VMEM_H

#include "phantom_env.h"
#include <rm_session/connection.h>
#include <region_map/client.h>
#include <dataspace/client.h>
#include <base/attached_ram_dataspace.h>
#include <base/heap.h>

using namespace Genode;

static const addr_t OBJECT_SPACE_SIZE = 0x80000000;
static const addr_t OBJECT_SPACE_START = 0x80000000;
static const size_t PAGE_SIZE = 4096;

/**
 * Region-manager fault handler resolves faults by attaching new dataspaces
 */
class Phantom::Local_fault_handler : public Genode::Entrypoint
{
private:
    Env &_env;
    Region_map &_region_map;
    Signal_handler<Local_fault_handler> _handler;
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
        Dataspace_capability ds = _env.ram().alloc(PAGE_SIZE);
        _region_map.attach_at(ds, state.addr & ~(PAGE_SIZE - 1));

        log("returning from handle_fault");
    }

public:
    Local_fault_handler(Genode::Env &env, Region_map &region_map)
        : Entrypoint(env, sizeof(addr_t) * 2048, "local_fault_handler",
                     Affinity::Location()),
          _env(env),
          _region_map(region_map),
          _handler(*this, *this, &Local_fault_handler::_handle_fault)
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

struct Phantom::Vmem_adapter
{
    const unsigned int _phys_rm_size = 1024 * 1024 * 512;

    Env &_env;
    Rm_connection _rm{_env};

    // Attached to env's rm
    Region_map_client _obj_space{_rm.create(OBJECT_SPACE_SIZE)};

    // Not attached fully, but pages from it supposed to be mapped on object space
    Region_map_client _pseudo_phys_rm{_rm.create(_phys_rm_size)};
    // Heap (allocator) that allocates dataspace per each allocated page and maps it on _pseudo_phys_rm
    Sliced_heap _pseudo_phys_heap{_env.ram(), _pseudo_phys_rm};

    Vmem_adapter(Env &env) : _env(env)
    {

        Phantom::Local_fault_handler fault_handler(env, _obj_space);
        // ATTENTION! _obj_space is attached to the env's rm!
        void *ptr_obj_space = env.rm().attach(_obj_space.dataspace(), 0, 0, true, OBJECT_SPACE_START, false, true);
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

    void map_page(addr_t phys_addr, addr_t virt_addr)
    {
        if (virt_addr >= OBJECT_SPACE_START && virt_addr < OBJECT_SPACE_START + OBJECT_SPACE_SIZE)
        {
            // TODO : Error handling
            _obj_space.attach_at(_pseudo_phys_rm.dataspace(), virt_addr - OBJECT_SPACE_START, PAGE_SIZE, phys_addr);
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