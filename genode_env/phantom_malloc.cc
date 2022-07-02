extern "C" {
    #include <ph_malloc.h>
}

#include <stdlib.h>


void *ph_malloc(size_t size){
    return malloc(size);
}

void ph_free(void * addr){
    free(addr);
}

void *ph_calloc(size_t n_elem, size_t elem_size){
    calloc(n_elem, elem_size)
}