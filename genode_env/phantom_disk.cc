#include <base/log.h>
// #include <stdio.h>
// #include <stddef.h>
#include <stdlib.h>
#include "phantom_env.h"

extern "C"
{

#include <genode_disk.h>

// Required to access vmem functions
#include <hal.h>

    static int seq_number = 0;
    static genode_disk_dev_t vdev;
    static char vdev_name[32] = "GenodeDisk0";

    void driver_genode_update_disk_dev(genode_disk_dev_t *dest)
    {
        auto info = main_obj->_disk.getInfo();

        dest->block_count = info.block_count;
        dest->block_size = info.block_size;
        dest->name = "GenodeDisk0";
    }

    void driver_genode_disk_read(genode_disk_dev_t *vd, void *dest, int block_no, size_t len)
    {
        (void)vd;

        if (len % PAGE_SIZE != 0)
        {
            Genode::warning("Incorrect len for disk read, not block aligned! (addr=%d block=%d len=%d)", dest, block_no, len);
            return;
        }

        // TODO : Do we need to sync here?
        main_obj->_disk.submit(Disk_backend::Operation::READ, true, block_no, len, dest);
    }

    void driver_genode_disk_write(genode_disk_dev_t *vd, void *src, int block_no, size_t len)
    {
        (void)vd;

        if (len % PAGE_SIZE != 0)
        {
            Genode::warning("Incorrect len for disk write, not block aligned! (addr=%d block=%d len=%d)", src, block_no, len);
            return;
        }

        // TODO : Do we need to sync here?
        main_obj->_disk.submit(Disk_backend::Operation::READ, true, block_no, len, src);
    }
}