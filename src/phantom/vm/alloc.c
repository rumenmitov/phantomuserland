/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * VM memory allocator
 *
**/


#define DEBUG_MSG_PREFIX "vm.alloc"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <ph_string.h>

#include <vm/alloc.h>
#include <vm/object_flags.h>
#include <kernel/stats.h>
#include <kernel/page.h>
#include <kernel/vm.h>
#include <kernel/debug_graphical.h>


static int object_allocator_inited = 0;

#define debug_memory_leaks 0
#define debug_allocation 0


#define DEBUG_PRINT(a)   if (debug_allocation) ph_printf(a)
#define DEBUG_PRINT1(a,b)  if (debug_allocation) ph_printf(a,b)



static void init_free_object_header( pvm_object_storage_t *op, unsigned int size );

// TODO Object alloc - gigant lock for now. This is to be redone with separate locks for buckets/arenas.
static hal_mutex_t  _vm_alloc_mutex;
hal_mutex_t  *vm_alloc_mutex; // used in gc.c and i_wasm.c


// Allocator and GC work in these bounds. NB! - pvm_object_space_end is OUT of arena
static void * pvm_object_space_start;
static void * pvm_object_space_end;

void * get_pvm_object_space_start() { return pvm_object_space_start; }
void * get_pvm_object_space_end() { return pvm_object_space_end; }


//
#define ARENAS 5
static void * start_a[ARENAS];
static void * end_a[ARENAS];
// Last position where allocator finished looking for objects
static void * curr_a[ARENAS];


// Names helper
static const char* name_a[ARENAS] = { "root, static", "stack", "int", "small", "large" };
// Partition
#if debug_allocation
static int percent_a[ARENAS] = { 15, 15, 2, 54, 14 };  // play with numbers
//static int percent_a[ARENAS] = { 15, 15, 20, 25, 25 };  // play with numbers
#else
//static int percent_a[ARENAS] = { 15, 45, 10, 15, 15 };  // play with numbers
static int percent_a[ARENAS] = { 15, 15, 2, 28, 40 };
#endif


static inline int find_arena(unsigned int size, unsigned int flags, bool saturated)
{
    int arena = 0; // root|saturated|code|class|interface|Large - nearly constant

    if (flags & (PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME|PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME))
        arena = 1; //fast
    else if (flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT)
        arena = 2; //small and fast
    else if (saturated)
        arena = 0; //never dies
    else if (flags & (PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE|PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS|PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE))
        arena = 0; //code|class|interface - never dies?
    else if (size < 1000)
        arena = 3; //small
    else // >1000
        arena = 4; //large

    return arena;
}

static inline int find_arena_by_address(void *memaddr)
{
    int i;
    for( i = 0; i < ARENAS; i++ )
    {
        if( (memaddr >= start_a[i]) && (memaddr < end_a[i]) )
            return i;
    }
    panic("arena not found for addr %p", memaddr);
    // return -1;
}


static inline unsigned int round_size(unsigned int size, int arena)
{
    (void) arena;
    size = (((size - 1) / 8) + 1) * 8 ;  //align 8 bytes

    // for reference: minimal object (int) = 36 bytes

//    if (arena == 3) //small strings
//        size = (((size - 1)/ 72) + 1) * 72 ;  //align 72 bytes (typical sizes: 40 and 68 bytes) - avoid fragmentation

    return size;
}

pvm_object_storage_t *get_root_object_storage() { return pvm_object_space_start; }

// TODO must be rewritten - arena properties must be kept in persistent memory
static void init_arenas( void * _pvm_object_space_start, unsigned int size )
{
    pvm_object_space_start = _pvm_object_space_start;
    pvm_object_space_end = pvm_object_space_start + size;

    void * cur = _pvm_object_space_start;
    int percent_100 = 0;
    int i;
    for( i = 0; i < ARENAS; i++) {
        start_a[i] = cur;
        curr_a[i] = cur;
        cur += (size / 800) * percent_a[i] * 8;  //align 8 bytes
        percent_100 += percent_a[i];
        end_a[i] = cur;
    }
    assert(percent_100 == 100); //check twice!
    end_a[ARENAS-1] = pvm_object_space_start + size; //to be exact
}


// Initialize the heap
void pvm_alloc_clear_mem()
{
    assert( pvm_object_space_start != 0 );
    int i;
    for( i = 0; i < ARENAS; i++) {
        init_free_object_header((pvm_object_storage_t *)start_a[i], end_a[i] - start_a[i]);
    }
}


// Initialize the heap, prepare
void pvm_alloc_init( void * _pvm_object_space_start, unsigned int size )
{
    assert(_pvm_object_space_start != 0);
    assert(size > 0);

    init_arenas(_pvm_object_space_start, size);


    //init_gc();  // here, if needed

    object_allocator_inited = 1;
}


void pvm_alloc_threaded_init(void)
{
    if( hal_mutex_init( &_vm_alloc_mutex, "ObjAlloc" ) )
        panic("Can't init allocator mutex");

    vm_alloc_mutex = &_vm_alloc_mutex;
}


static void init_object_header(pvm_object_storage_t *op, unsigned int size)
{
    assert( size >= sizeof(pvm_object_storage_t) );

    op->_ah.object_start_marker = PVM_OBJECT_START_MARKER;
    op->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED;
    op->_ah.gc_flags = 0;
    op->_ah.refCount = 1;
    op->_ah.exact_size = size;
}

static void init_free_object_header(pvm_object_storage_t *op, unsigned int size)
{
    assert( size >= sizeof(pvm_object_storage_t) );

    op->_ah.object_start_marker = PVM_OBJECT_START_MARKER;
    op->_ah.alloc_flags = PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE;
    op->_ah.gc_flags = 0;
    op->_ah.refCount = 0;
    op->_ah.exact_size = size;
}


#define PVM_MIN_FRAGMENT_SIZE  (sizeof(pvm_object_storage_t) + sizeof(int) )      /* should be a minimal object size at least */


// returns allocated object
static pvm_object_storage_t *alloc_eat_some(pvm_object_storage_t *op, unsigned int size)
{
    assert( op->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( op->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE );

    assert(op->_ah.exact_size >= size);
    unsigned int surplus = op->_ah.exact_size - size;
    if (surplus < PVM_MIN_FRAGMENT_SIZE) {
        // don't break in too small pieces
        init_object_header(op, op->_ah.exact_size);  //update alloc_flags
        return op;
    }

    init_object_header(op, size);  //update size and alloc_flags

    void *o = (void*)op + size;
    pvm_object_storage_t *opppa = (pvm_object_storage_t *)o;
    init_free_object_header(opppa, surplus);
    return op;
}


// try to collapse current with next objects until they are free
static void alloc_collapse_with_next_free(pvm_object_storage_t *op, unsigned int need_size, void * end, int arena)
{
    assert( op->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( op->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE );

    unsigned int size = op->_ah.exact_size;
    do {
        void *o = (void *)op + size;
        pvm_object_storage_t *opppa = (pvm_object_storage_t *)o;
        if (
             ( o < end )  &&
             ( (void *)opppa < end )  &&
             ( opppa->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE )  &&
             ( size < need_size * 32 ) //limit page in amount
           )
        {
            size += opppa->_ah.exact_size;
            DEBUG_PRINT("^");
        } else {
            break;
        }
    } while(1);

    if (size > op->_ah.exact_size)
    {
        init_free_object_header(op, size);  //update exact_size
        DEBUG_PRINT1("%d", arena);
    }
}

// suspected to break object land - alloc_collapse_with_next_free: assertion 'op->_ah.object_start_marker == PVM_OBJECT_START_MARKER' failed
void pvm_collapse_free(pvm_object_storage_t *op)
{
#if VM_UNMAP_UNUSED_OBJECTS
    if(vm_alloc_mutex) hal_mutex_lock( vm_alloc_mutex );  // TODO avoid Giant lock

    int arena = find_arena_by_address(op);
    if( arena < 0 )
        return;

    void *start = (void *)op;
    void *end = ( start < curr_a[arena] ) ? curr_a[arena] : end_a[arena];
    // Attempt to collapse up to 10 pages
    alloc_collapse_with_next_free( op, PAGE_SIZE*10, end, arena);

    addr_t data_start = (addr_t) &(op->da);
    size_t data_size  = op->_ah.exact_size;

    data_size -= __offsetof(pvm_object_storage_t, da);


    if( data_size > PAGE_SIZE )
    {
        addr_t page_start = PAGE_ALIGN(data_start);

        data_size -= ( page_start - data_start );

        int maxp = 16; // not forever!

        while( (data_size > PAGE_SIZE) && (maxp-- > 0) )
        {
            vm_map_page_mark_unused(page_start);
            data_size  -= PAGE_SIZE;
            page_start += PAGE_SIZE;
        }
    }

    if(vm_alloc_mutex) hal_mutex_unlock( vm_alloc_mutex );  // TODO avoid Giant lock
#endif
}


// walk through
static pvm_object_storage_t *alloc_wrap_to_next_object(pvm_object_storage_t *op, void * start, void * end, int *wrap, int arena)
{
    assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER);

    void *o = (void *)op + op->_ah.exact_size;

    if( o >= end )
    {
        assert(o <= end);
        o = start;
        (*wrap)++;
        DEBUG_PRINT("\n(alloc wrap)");
        DEBUG_PRINT1("%d", arena);
    }
    return (pvm_object_storage_t *)o;
}



static inline int pvm_alloc_is_object(pvm_object_storage_t *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER;
}

static inline int pvm_alloc_is_free_object(pvm_object_storage_t *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER
                 &&  o->_ah.alloc_flags == PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE ;
}

bool pvm_object_is_allocated_light(pvm_object_storage_t *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER
            && o->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED;
}

// maximal test:
bool pvm_object_is_allocated(pvm_object_storage_t *o)
{
    return o->_ah.object_start_marker == PVM_OBJECT_START_MARKER
            && o->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED
            //o->_ah.gc_flags == 0;
            && o->_ah.refCount > 0
            && o->_ah.exact_size >= ( o->_da_size + sizeof(pvm_object_storage_t) )
            && o->_ah.exact_size <  ( o->_da_size + sizeof(pvm_object_storage_t) + PVM_MIN_FRAGMENT_SIZE )
            && o->_ah.exact_size <= ( pvm_object_space_end - (void*)o ) ;
}

void pvm_object_is_allocated_assert(pvm_object_storage_t *o)
{
    assert( o->_ah.object_start_marker == PVM_OBJECT_START_MARKER );
    assert( o->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED );
    //o->_ah.gc_flags == 0;
    assert( o->_ah.refCount > 0 );
    assert( o->_ah.exact_size >= ( o->_da_size + sizeof(pvm_object_storage_t) ) );
    assert( o->_ah.exact_size <  ( o->_da_size + sizeof(pvm_object_storage_t) + PVM_MIN_FRAGMENT_SIZE ) );
    assert( o->_ah.exact_size <= (pvm_object_space_end - (void*)o) );
}




// Find a piece of mem of given or bigger size. Linear allocation.
static pvm_object_t pvm_find(unsigned int size, int arena)
{

#define CURR_POS  curr_a[arena]

//    static int rest_cnt = 0;
//
//    if(size < 64 && rest_cnt++ > 2000)
//    {
//        CURR_POS = start_a[arena];
//        rest_cnt = 0;
//    }
    // a kind of optimization above



    pvm_object_t result = 0;

    pvm_object_t start = start_a[arena];
    pvm_object_t end = end_a[arena];

    int wrap = 0;
    pvm_object_t curr = CURR_POS;

    while(result == 0)
    {
        if( wrap > 1 ) {
            return 0;
        }

        if( PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED & curr->_ah.alloc_flags )
        {
            DEBUG_PRINT("a");
            // Is allocated? Go to the next one.
            curr = alloc_wrap_to_next_object(curr, start, end, &wrap, arena);
            continue;
        }

        if( PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE != curr->_ah.alloc_flags ) // refcount == 0, but refzero or in buffer or both
        {
            DEBUG_PRINT("(c)");
            refzero_process_children( curr );
            // Supposed to be free here
        }

        // now free
        assert( PVM_OBJECT_AH_ALLOCATOR_FLAG_FREE == curr->_ah.alloc_flags );

        if (curr->_ah.exact_size < size) {
            alloc_collapse_with_next_free(curr, size, end, arena);
            // try again -
            if (curr->_ah.exact_size < size) {
                DEBUG_PRINT("|");
                curr = alloc_wrap_to_next_object(curr, start, end, &wrap, arena);
                continue;
            }
        }

        result = alloc_eat_some(curr, size);
        //break;
    }

    assert(result != 0);
    DEBUG_PRINT("+");

    CURR_POS = alloc_wrap_to_next_object(result, start, end, &wrap, arena);
    //CURR_POS = result;
    return result;
}



#define PVM_GC_ENABLE 1


static pvm_object_storage_t * pool_alloc(unsigned int size, int arena)
{
    pvm_object_storage_t * data = 0;

    if(vm_alloc_mutex) hal_mutex_lock( vm_alloc_mutex );  // TODO avoid Giant lock

#if PVM_GC_ENABLE
    int ngc = 1;
#endif
    do {
        data = pvm_find(size, arena);

#if PVM_GC_ENABLE
        if(data)
            break;

        if(ngc-- <= 0)
            break;

        ph_printf("\n out of mem looking for %d bytes, arena %d. Run GC... \n", size, arena);

        /*
         * NB!  In fact, it is a huge risk to call GC here:  being called whithin constructor
         * GC will free objects just allocated but not yet linked to parents. Highly destructive!
         *
         */
        if(vm_alloc_mutex) hal_mutex_unlock( vm_alloc_mutex );
        run_gc();
        if(vm_alloc_mutex) hal_mutex_lock( vm_alloc_mutex );
#else
        break; //skip GC, until we bring context to the allocator
#endif
    } while(1);

    if(vm_alloc_mutex) hal_mutex_unlock( vm_alloc_mutex );

    return data;
}


//allocation statistics:
#define max_stat_size 4096
static long created_o[ARENAS][max_stat_size+1];
static long created_large_o[ARENAS];
static long used_o[ARENAS][max_stat_size+1];
static long used_large_o[ARENAS];


pvm_object_storage_t * pvm_object_alloc( unsigned int data_area_size, unsigned int flags, bool saturated )
{
    unsigned int size = sizeof(pvm_object_storage_t) + data_area_size;

    pvm_object_storage_t * data;

    int arena = find_arena(size, flags, saturated);
    size = round_size(size, arena);

    data = pool_alloc(size, arena);

    if( data == 0 )
    {
        int i;
        // Try any pool now
        for( i = ARENAS-1; i>= 0; i-- )
        {
            data = pool_alloc(size, i);
            if( data != 0 )
                goto found;
        }

        pvm_memcheck();
        // TODO VM throw
        panic("out of persistent mem looking for %d bytes, arena %d", size, arena );
    }

found:
    data->_da_size = data_area_size;
    data->_flags = flags;
    if (saturated)
        ref_saturate_p(data);

    debug_catch_object("new", data);

    // TODO remove it here - memory must be cleaned some other, more effective way
    // ! clean on object deletion (can be done at idle time)
    ph_memset( data->da, 0, data_area_size );

    //stat
    if (size <= max_stat_size)
        created_o[arena][size]++;
    else
        created_large_o[arena]++;

    // kern stat
    STAT_INC_CNT( OBJECT_ALLOC );
    return data;
}



/*void object_delete( pvm_object_storage * o )
{
}*/

// -----------------------------------------------------------------------


static inline pvm_object_t pvm_next_object(pvm_object_t op)
{
    //assert(op->_ah.object_start_marker == PVM_OBJECT_START_MARKER);

    void *o = (void *)op;
    o += op->_ah.exact_size;
    return (pvm_object_t )o;
}


// -----------------------------------------------------------------------
// Memory walk/check code
// -----------------------------------------------------------------------


static int memcheck_one(unsigned int i, void * start, void * end);
static void memcheck_print_histogram(unsigned int i);


/*
 *
 * Scan all the memory and check for inconsistencies.
 * Supposed to be called on kernel start before any
 * thread run.
 *
 */

int pvm_memcheck()
{
    unsigned int i;
    for( i = 0; i < ARENAS; i++)
    {
        ph_printf("\n Arena #%d [%s] \n", i, name_a[i]);
        memcheck_one(i, start_a[i], end_a[i]);
    }

    return 0;
}

static void printmemsize( unsigned long sz, char *name )
{
    if( sz > 1024*1024 )
        ph_printf("%ldM ", sz / (1024*1024));
    sz %= 1024*1024;

    if( sz > 1024 )
        ph_printf("%ldK ", sz / (1024));
    sz %= 1024;

    ph_printf("%ld %s", sz, name );
}

static int memcheck_one(unsigned int i, void * start, void * end)
{
    used_large_o[i] = 0; //reset
    unsigned int size;
    for( size = 0; size <= max_stat_size; size++)
        used_o[i][size] = 0; //reset

    unsigned long used = 0, free = 0, objects = 0, largest = 0;

    pvm_object_t curr = start;

    ph_printf("Memcheck: checking object memory allocation consistency (at %p, %ld bytes)\n", start, (long)(end - start) );

    while(((void *)curr) < end)
    {
        if(!pvm_alloc_is_object(curr))
        {
            //ph_printf("Memcheck: %ld objects, memory: %ld used, %ld free, %ld largest\n", objects, used, free, largest );
            //return 0;
            ph_printf("Memcheck: not an object at %p\n", curr);
            break;
        }

        // Is an object.

        if( curr->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED )
        {
            if(!pvm_object_is_allocated(curr))  //more tests
            {
                ph_printf("Memcheck: corrupted allocated object at %p\n", curr);
                break;
            }
            if (curr->_ah.exact_size <= max_stat_size)
                used_o[i][curr->_ah.exact_size]++;
            else
                used_large_o[i]++;

            used += curr->_ah.exact_size;
            objects++;
        }
        else
        {
            free += curr->_ah.exact_size;
            if( curr->_ah.exact_size > largest )
                largest = curr->_ah.exact_size;
        }


        //curr = pvm_next_object(curr);
        curr = (pvm_object_t)( ((void *)curr) + curr->_ah.exact_size );
    }

    //ph_printf("Memcheck: %ld objects, memory: %ld used, %ld free\n", objects, used, free );
    //ph_printf("Memcheck: %ld objects, memory: %ld used, %ld free, %ld largest ", objects, used, free, largest );
    ph_printf("Memcheck: %ld objects, memory:  ", objects );
    printmemsize( used, "used, " );
    printmemsize( free, "free, " );
    printmemsize( largest, "largest" );
    ph_printf("\n");

    if((void *)curr == end)
    {
        ph_printf("Memcheck: reached exact arena end at %p (%ld bytes used)\n", curr, (long)(((void *)curr) - start) );
        if (debug_memory_leaks) memcheck_print_histogram(i);
        return 0;
    }

    ph_printf("\n\n-----------------\nMemcheck ERROR: reached out of arena end at 0x%p (%ld bytes size)\n-----------------\n\n", curr, (long) (((void *)curr) - start) );
    return 1;
}

// count persistent objects in the specified memory range
// intended for debugging purposes
static int64_t count_objects(void *start, void *end)
{
    int64_t count = 0;
    pvm_object_t curr = start;

    while(((void *)curr) < end) {
        if(!pvm_alloc_is_object(curr)) return -1;

        if(curr->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED) {
            if(!pvm_object_is_allocated(curr)) return -1;
            count++;
        }

        curr = (pvm_object_t)( ((void *)curr) + curr->_ah.exact_size );
    }

    return count;
}

// scan entire object land for objects and count them
// intended for debugging purposes
// will likely fail if any PVM threads are active
int64_t pvm_count_allocated_objects() {
    int64_t object_count = count_objects(pvm_object_space_start, pvm_object_space_end);
    if (object_count < 0) {
        SHOW_ERROR0(0, "Failed! (did not lock memory before scan?)");
    } else {
        SHOW_INFO(0, "Total persistent objects currently allocated: %d", object_count);
    }
    return object_count;
}

static void memcheck_print_histogram(unsigned int arena)
{
    if (arena == 0) return; //nothing interesting

    if (created_large_o[arena] > 0)
        ph_printf(" large objects: now used %ld, was allocated %ld\n", used_large_o[arena], created_large_o[arena]);

    ph_printf(" small objects: size, now used, was allocated\n");
    unsigned int size;
    for( size = 0; size <= max_stat_size; size++)
    {
        if(created_o[arena][size] > 0)
            ph_printf("   %6d %6ld %12ld \n", size, used_o[arena][size], created_o[arena][size]);
    }
}




//---------------------------------------------------------------------------
// Debug window - mem map
//---------------------------------------------------------------------------

#include <video/window.h>

//static rgba_t calc_object_pixel_color( int elem, int units_per_pixel );
static void paint_arena_memory_map(window_handle_t w, rect_t *r, int a );


/**
 * 
 * \brief Generic painter for any allocator using us.
 * 
 * Used in debug window.
 * 
 * \param[in] w Window to draw to
 * \param[in] r Rectangle to paint inside
 * 
**/

static int vertical_start_for_arena[ARENAS];
static int vertical_pixels_per_arena[ARENAS];

void paint_object_memory_map(window_handle_t w, rect_t *r )
{
    int i;

    if(!object_allocator_inited) return;
    
    //pvm_memcheck();

    if(vertical_pixels_per_arena[0] == 0)
    {
        int y = 0;

        size_t omem_bytes = pvm_object_space_end - pvm_object_space_start;

        for( i = 0; i < ARENAS; i++) 
        {
            size_t arena_bytes = end_a[i] - start_a[i];
            size_t percentage = arena_bytes * 100 / omem_bytes;

            int vpixels = (r->ysize * percentage) / 100;
            LOG_FLOW( 0, "omem=%uK, arena mem=%uK, mem %%=%u, ysize %d, ysize part = %d",
                omem_bytes/1024, arena_bytes/1024, percentage, r->ysize, vpixels
                );

            vertical_start_for_arena[i] = y;
            vertical_pixels_per_arena[i] = vpixels;
            y += vpixels;
        }
    }

    // now paint
    for( i = 0; i < ARENAS; i++) 
    {
        rect_t ar = *r;
        ar.y = vertical_start_for_arena[i];
        ar.ysize = vertical_pixels_per_arena[i];

        hal_mutex_lock( vm_alloc_mutex );  // TODO avoid Giant lock - must be per arena
        paint_arena_memory_map( w, &ar, i );
        hal_mutex_unlock( vm_alloc_mutex );
    }

}

void paint_arena_memory_map(window_handle_t w, rect_t *r, int a )
{
    void * mem_start = start_a[a];
    void * mem_end = end_a[a];
    size_t mem_size = mem_end - mem_start;

    int pixels = r->xsize * r->ysize;
    int units_per_pixel =  1 + ((mem_size-1) / pixels);

    //int prev_object_crosses_pixel_and_not_free = 0;
    pvm_object_t curr = mem_start;

    void *this_pixel_start = mem_start;
    void *this_pixel_end = mem_start+units_per_pixel;

    LOG_FLOW( 0, "arena %d, start=%p, end=%p", a, mem_start, mem_end );

    int x, y;
    for( y = 0; y < r->ysize; y++ )
    {
        for( x = 0; x < r->xsize; x++ )
        {
            int cnt_used = 0;
            int cnt_free = 0;

            pvm_object_t next = ((void *)curr) + curr->_ah.exact_size;

            LOG_FLOW( 11, "pixel %d/%d, start=%p, end=%p", x, y, this_pixel_start, this_pixel_end );

            while( ((void *)next) < this_pixel_end )
            {
                assert( ((void *)curr) < this_pixel_end );

                if( curr->_ah.object_start_marker != PVM_OBJECT_START_MARKER ) 
                {
                    LOG_FLOW( 0, "object %p BROKEN", curr );
                    return;
                }

                assert( curr->_ah.exact_size != 0 );

                if( curr->_ah.alloc_flags & PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED ) 
                    cnt_used++;
                else 
                    cnt_free++;

                curr = next;
                next = ((void *)curr) + curr->_ah.exact_size; 
            }

            w_draw_pixel( w, x + r->x, y + r->y, 
                (cnt_used == 0) ? COLOR_BLUE : ( (cnt_free == 0) ? COLOR_RED : COLOR_GREEN )
                );

            this_pixel_start += units_per_pixel;
            this_pixel_end   += units_per_pixel;

        }
    }

}
/*
static rgba_t calc_object_pixel_color( int elem, int units_per_pixel )
{
    vm_page *ep = vm_map_map + elem;

    int state = 0; // 0 = empty, 1 = partial, 2 = used
    int bits = 0;
    int do_io = 0;

    int i;
    for( i = 0; i < units_per_pixel; i++, ep++ )
    {
        if( 0 == ep->flag_phys_mem ) continue; // empty, no change
        state = 2; // full
        bits += 1;

        if( ep->flag_pager_io_busy ) 
        {
            do_io = 1;
            //lprintf("io %d ", elem+i);
        }
    }

    if(do_io) return COLOR_YELLOW;

    switch(state)
    {
        case 0: return COLOR_BLUE;
        
        case 1: 
        {
            //return COLOR_LIGHTGREEN;
            rgba_t c = COLOR_LIGHTGREEN;
            // lighter = less used
            c.g = 0xFF - (bits * 0xFF / (units_per_pixel * sizeof(map_elem_t) * 8));
            return c;
        }

        case 2: return COLOR_LIGHTRED;

        default: return COLOR_BLACK;
    }
}
*/



