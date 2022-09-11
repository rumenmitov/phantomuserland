#include "phantom_env.h"
#include <base/stdint.h>

extern "C"
{
#include <ph_malloc.h>
}

extern "C" void *ph_malloc(size_t size)
{
    // Storing the size of the allocation in the beginning
    // since _heap.free() requires the size as well

    size_t total_size = sizeof(size_t) + size;

    void *original_addr = main_obj->_heap.alloc(total_size);

    // Writing the size
    *((size_t *)original_addr) = size;

    // Returning an adjusted pointer
    size_t *adjusted_addr = ((size_t *)original_addr) + 1;
    Genode::log("ph_malloc: addr=", original_addr, ", size=", size);

    return (void *)adjusted_addr;
}

extern "C" void ph_free(void *addr)
{
    // addr should be not the beginning of the allocation, but allocation + sizeof(size_t)

    size_t *adjusted_addr = (size_t *)addr;
    void *original_addr = (void *)(adjusted_addr - 1);

    size_t size = *((size_t *)original_addr);
    Genode::log("ph_free: addr=", original_addr, ", size=", size);

    main_obj->_heap.free(original_addr, size + sizeof(size_t));
}

extern "C" void *ph_calloc(size_t n_elem, size_t elem_size)
{
    return ph_malloc(n_elem * elem_size);
}