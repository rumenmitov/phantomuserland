/*
 * Memory allocator for TinyGL
 */
#include "zgl.h"

/* modify these functions so that they suit your needs */

void gl_free(void *p)
{
    ph_free(p);
}

void *gl_malloc(int size)
{
    return ph_malloc(size);
}

void *gl_zalloc(int size)
{
    return ph_calloc(1, size);
}
