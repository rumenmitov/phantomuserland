#define DEBUG_MSG_PREFIX "GenodeDiskIO"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include "genode_disk.h"

// Required to access vmem functions
#include <hal.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

static int seq_number = 0;
static genode_disk_dev_t vdev;
static char vdev_name[32] = "GenodeDisk0";

phantom_device_t *driver_genode_disk_probe()
{

    vdev.name = vdev_name;

    phantom_device_t *dev = (phantom_device_t *)malloc(sizeof(phantom_device_t));
    dev->name = "Genode disk";
    dev->seq_number = seq_number++;
    dev->drv_private = &vdev;

    return dev;
}

static phantom_disk_partition_t *phantom_create_genode_partition_struct(long size, genode_disk_dev_t *vd)
{
    phantom_disk_partition_t *ret = phantom_create_partition_struct(0, 0, size);

    ret->asyncIo = driver_genode_disk_asyncIO;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;
    ret->specific = vd;

    // important since it is used to break request in the parts
    ret->block_size = vd->block_size;

    // ret->dequeue = vioDequeue;
    // ret->raise = vioRaise;
    // ret->fence = vioFence;
    // ret->trim = vioTrim;

    //strlcpy( ret->name, "virtio", PARTITION_NAME_LEN );
    strlcpy(ret->name, vd->name, PARTITION_NAME_LEN);

    return ret;
}

// Initialization and registration of a disk inside the system
void driver_genode_disk_init()
{

    // Creating and initializing partition

    phantom_disk_partition_t *part = phantom_create_genode_partition_struct(vdev.block_count, &vdev);

    if (part == 0)
    {
        SHOW_ERROR0(0, "Failed to create whole disk partition");
        return;
    }

    errno_t err = phantom_register_disk_drive(part);

    if (err)
    {
        SHOW_ERROR0(0, "Failed to register Genode disk!");
        return;
    }
}

// Main function for performing I/O
int driver_genode_disk_asyncIO(struct phantom_disk_partition *part, pager_io_request *rq)
{

    // assert(part->specific != 0); // TODO : Causes a compilation error!!!

    // Temp! Rewrite!
    //assert(p->base == 0 );

    int block_size = vdev.block_size;

    genode_disk_dev_t *vd = (genode_disk_dev_t *)part->specific;

    int sect = rq->blockNo;
    int n = rq->nSect;
    physaddr_t pa = rq->phys_page;

    int length_in_bytes = rq->nSect * vdev.block_size;

    rq->parts = n;

    // TODO : Rework to be more efficient
    // TODO : IT SHOULD NOT HAPPEN IN OBJECT SPACE! Use another allocator
    // Need to write to pseudo phys page. It means that we need to map it first
    void *temp_mapping = NULL;
    hal_alloc_vaddress(&temp_mapping, block_size);
    hal_pages_control(pa, temp_mapping, length_in_bytes, page_map, rq->flag_pagein ? page_rw : page_ro);

    // Now we can perform IO
    if (rq->flag_pageout)
        driver_genode_disk_write(vd, pa, rq->blockNo, rq->nSect);
    else
        driver_genode_disk_read(vd, pa, rq->blockNo, rq->nSect);

    // Now we can remove temporary mapping and dealloc address
    hal_pages_control(0, temp_mapping, length_in_bytes, page_unmap, page_rw);
    hal_free_vaddress(temp_mapping, block_size);

    // TODO : error handling

    return 0;
}