extern "C" {
    #include <ph_malloc.h>
}

#include <base/stdint.h>
#include <stdlib.h>

// using namespace Genode;

extern "C" void *ph_malloc(size_t size){
    return malloc(size);
}

extern "C" void ph_free(void * addr){
    free(addr);
}

extern "C" void *ph_calloc(size_t n_elem, size_t elem_size){
    return calloc(n_elem, elem_size);
}