#include "genode_disk.h"

// #include <debug_ext.h>

#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

static const int disk_size = 1024 * 1024 * 256;

static int seq_number = 0;
static genode_disk_dev_t vdev;

phantom_device_t driver_genode_disk_probe(){

	vdev.name = "GenodeDisk0";

    phantom_device_t * dev = (phantom_device_t *)malloc(sizeof(phantom_device_t));
    dev->name = "Genode disk";
    dev->seq_number = seq_number++;
    dev->drv_private = &vdev;
}


static phantom_disk_partition_t *phantom_create_genode_partition_struct( long size, genode_disk_dev_t *vd )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size);

    ret->asyncIo = driver_genode_disk_asyncIO;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;

    ret->specific = vd;

    // ret->dequeue = vioDequeue;
    // ret->raise = vioRaise;
    // ret->fence = vioFence;
    // ret->trim = vioTrim;

    //strlcpy( ret->name, "virtio", PARTITION_NAME_LEN );
    strlcpy( ret->name, vd->name, PARTITION_NAME_LEN );

    return ret;
}

// Initialization and registration of a disk inside the system
void driver_genode_disk_init(){

    int size = disk_size;

    // Creating and initializing partition

	phantom_disk_partition_t * part = phantom_create_genode_partition_struct( disk_size, &vdev );

    if(part == 0)
    {
        SHOW_ERROR0( 0, "Failed to create whole disk partition" );
        return;
    }

    errno_t err = phantom_register_disk_drive(part);

    if(err)
    {
        SHOW_ERROR( 0, "Failed to register Genode disk!");
        return;
    }
}

// Main function for performing I/O
int driver_genode_disk_asyncIO(struct phantom_disk_partition *part, pager_io_request *rq ){
    
    // assert(part->specific != 0); // TODO : Causes a compilation error!!!

    // Temp! Rewrite!
    //assert(p->base == 0 );

    genode_disk_dev_t *vd = (genode_disk_dev_t *)part->specific;


	int sect = rq->blockNo;
	int n = rq->nSect;
	physaddr_t pa = rq->phys_page;

	rq->parts = n;

	while(n--)
	{
		if( rq->flag_pageout )
			driver_genode_disk_write(vd, pa, 512, rq, sect);
		else
			driver_genode_disk_read (vd, pa, 512, rq, sect);

		sect++;
		pa += 512;
	}

	return 0;
}

void driver_genode_disk_read(genode_disk_dev_t *vd, physaddr_t data, size_t len, pager_io_request *rq, int sect){
}

void driver_genode_disk_write(genode_disk_dev_t *vd, physaddr_t data, size_t len, pager_io_request *rq, int sect){
}


