#include "phantom_env.h"
#include <base/stdint.h>
#include <base/log.h>

extern "C"
{
#include <ph_malloc.h>
#include <ph_string.h>
}

extern "C" void *ph_malloc(size_t size)
{
    // Storing the size of the allocation in the beginning
    // since _heap.free() requires the size as well

    size_t total_size = sizeof(size_t) + size;


    // void *original_addr = main_obj->_heap.alloc(total_size);

    auto alloc_res = main_obj->_heap.try_alloc(total_size);

    if (!alloc_res.ok()){
        alloc_res.with_error([](Allocator::Alloc_error err){error(err);});
        return 0;
    }

    void* original_addr = nullptr;

    alloc_res.with_result(
        [&](void* addr){ original_addr = addr; }, 
        [&](Allocator::Alloc_error err){ error(err); }
    );

    // just to ensure it is safe
    if (original_addr == nullptr){
        error("ph_malloc: addr = nullptr!!!");
        return 0;
    }

    // Writing the size
    *((size_t *)original_addr) = size;

    // Returning an adjusted pointer
    size_t *adjusted_addr = ((size_t *)original_addr) + 1;
    Genode::log("ph_malloc: addr=", original_addr, "(", adjusted_addr, "), size=", size, " total_size=", total_size);
    
    // TODO : REMOVE!!!111
    // char* test_str = (char*) adjusted_addr;
    // Genode::log(test_str[0]);
    // Genode::log(test_str[1]);
    // Genode::log(test_str[2]);

    return (void *)adjusted_addr;
}

extern "C" void ph_free(void *addr)
{
    // addr should be not the beginning of the allocation, but allocation + sizeof(size_t)

    size_t *adjusted_addr = (size_t *)addr;
    void *original_addr = (void *)(adjusted_addr - 1);

    size_t size = *((size_t *)original_addr);
    Genode::log("ph_free: addr=", original_addr, "(", adjusted_addr, ", size=", size, "(", size + sizeof(size_t), ")");

    main_obj->_heap.free(original_addr, size + sizeof(size_t));
}

extern "C" void *ph_calloc(size_t n_elem, size_t elem_size)
{
    return ph_malloc(n_elem * elem_size);
}

// XXX : Allocates new area each time!
extern "C" void *ph_realloc(void *ptr, size_t size){
    // Genode::error("Unimplemented function ph_realloc()!");
    size_t old_size = *(((size_t*)ptr) - 1);
    Genode::log("ph_realloc: ", ptr, " size=", size, ", old_size=", old_size);
    void* new_area = ph_malloc(size);
    ph_memcpy(new_area, ptr, old_size);
    ph_free(ptr);
    return new_area;
}