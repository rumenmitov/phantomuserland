#ifndef PHANTOM_MALLOC
#define PHANTOM_MALLOC

#include <stdlib.h>

void *ph_malloc(size_t size);
void ph_free(void *);

void *ph_calloc(size_t n_elem, size_t elem_size);

// //! align = n bytes to align to
// void *ph_calloc_aligned(size_t n_elem, size_t elem_size, size_t align);

void *ph_realloc(void *ptr, size_t size);

#endif