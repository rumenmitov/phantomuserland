/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Pager (virtual memory/snapshot engine) IO interface.
 *
**/

#ifndef PAGER_H
#define PAGER_H

#include <phantom_disk.h>

#include "paging_device.h"
#include <pager_io_req.h>
#include "pagelist.h"
#include <spinlock.h>
#include <hal.h>


/* TODO: need queue sort to optimize head movement */


//void      pager_stop_io(); // called after io is complete
void        pager_free_io_resources(pager_io_request *req);

// Load superblock from disk (from default disk block, see DISK_STRUCT_SB_OFFSET_LIST)
// Panics if failed to load good superblock and failed to repair it
void        pager_get_superblock(void);
// Try to load root superblock from the specified disk block
// Return 0 if root superblock and its copies are fine, or if repair was successful
int         pager_try_get_superblock_from(disk_page_no_t root_block);
/* Write superblock and its copies to disk */
void        pager_update_superblock(void);

void        pager_fix_incomplete_format(void);
int         pager_fast_fsck(void);
int         pager_long_fsck(void);

/*
    ###########################################################
    ######################## DISK LEAK ########################
    ###########################################################

    Possible points of disk leak:
        - during snapshot first kickout phase: blocks are being removed 
            from freelist, and as another node of freelist is empty, the
            freelist head is updated, written to superblock and flushed 
            to disk. Interruption during or after this phase will result
            in freelist being much shorter than before. In fact, the 
            vulnerable interval is [kickout_start; snap_finish]; where
            snap_finish is when the snapshot is already commited, but
            the older snapshot is not freed yet
        - previous snapshot release: if the system is interrupted AFTER 
            the new snapshot was made, but BEFORE the old is deleted, 
            all the free blocks in the old snapshot do not get released,
            and the reference to the old snapshot is lost

*/


/*
    Adds the specified disk block to the freelist. In case the current node
    is full, the specified block is used as a new freelist node instead.
    If the freelist is not initialized *or* does not exist, it is created
    in the specified disk block and the superblock is updated.

    // I currently have no explanation on why is it made this way :/
    All changes to freelist / superblock are immediately written to disk,
    except: 
    - if the freelist exists and is initialized, and the current node is 
        *not* full at the moment of the call, no updates will be written to 
        disk 
    - if the current freelist block is full, then the current block gets 
        used as a new freelist head. While new value of freelist head is 
        written to superblock structure, the superblock is not flushed to 
        disk by this function in this case.

    Note that the disk block that is passed to this function may be 
    overwritten immediately (by a freelist node structure). Hence this 
    function should only be called if you are 100% sure that this disk 
    block is free and is not used by any current snapshots

    This function is called either when fixing an incomplete superblock (on 
    system boot) or by `pager_free_page` function, when freeing a disk block  

    // DO NOT use superblock free_start! -- WHY? [dz]
*/
void        pager_put_to_free_list( disk_page_no_t );
/*
    Adds free disk blocks to reserve list until it is full. Free blocks 
    are taken from the freelist, or from `free_start` if the freelist is
    empty. These disk blocks are removed from freelist / free_start as a
    result. 
    
    Reserve list of free disk blocks is a short array providing easy access
    to some available to use disk blocks.

    If the freelist head changes as a result of this function, this change
    is immediately written to superblock and to disk. However, the updated
    value of `free_start` is not written to disk by this function.

    This function is called by `pager_refill_free` (unconfirmed, but this
    might be called at any moment); and by `pager_alloc_page` function.
*/
void        pager_refill_free_reserve(void);
/*
    Makes the specified disk block a new head of the free list. This block
    is immediately overwritten with empty list node structure, with `next`
    node set to the current freelist head specified in the superblock.

    Note that this function modifies contents of `freelist_head` global 
    variable, but does not modify the value of current freelist head in the
    superblock structure. 

    This can potentially lead to:
    1) leakage of freed disk blocks: blocks will be written to the new
        node, event though it is not an actual freelist head
    2) freelist corruption: if the contents of `freelist_head` are not
        appropriately reloaded, and are written to the location of 
        actual freelist head, all the previous nodes in the list will
        be lost, and the resulting freelist is a single node that points
        to itself

    This function is called: when fixing an incomlete superblock (on system
    boot); and by `pager_put_to_free_list` function, when extending or 
    initializing the freelist.
*/
void        pager_format_empty_free_list_block( disk_page_no_t );
void        pager_init_free_list(void);
/*
    Writes current freelist head to disk
*/
void        pager_flush_free_list(void);


// NB! On pager_init() call pagefile must be good and healthy,
// so if one needs some check and fix, it must be done
// before.

#if PAGING_PARTITION
#include <disk.h>
void partition_pager_init(phantom_disk_partition_t *p);
#else
void        pager_init(paging_device * device); // {  }
#endif

void        pager_finish(void);


int         pager_interrupt_alloc_page(disk_page_no_t *out);
int         pager_alloc_page(disk_page_no_t *out);
void        pager_free_page( disk_page_no_t );

int         pager_can_grow(); // can I grow pagespace


void        pager_enqueue_for_pagein ( pager_io_request *p );
void        pager_enqueue_for_pagein_fast ( pager_io_request *p );
void        pager_enqueue_for_pageout( pager_io_request *p );
void        pager_enqueue_for_pageout_fast( pager_io_request *p );
void        pager_raise_request_priority(pager_io_request *p);
int         pager_dequeue_from_pageout(pager_io_request *p);

errno_t     pager_fence(void);

//void                pager_io_done(void);

long        pager_object_space_address(void);

phantom_disk_superblock *        pager_superblock_ptr();

void        pager_refill_free(void);

//! called to start new io
//! called under sema!
void        pager_start_io();



void        phantom_fsck(int do_rebuild );


void        phantom_free_snap(
        disk_page_no_t old_snap_start,
        disk_page_no_t curr_snap_start,
        disk_page_no_t new_snap_start
);


#endif // PAGER_H

