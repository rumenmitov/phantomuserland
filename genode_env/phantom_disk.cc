#include <base/log.h>
// #include <stdio.h>
// #include <stddef.h>
#include <stdlib.h>
#include "phantom_env.h"

using namespace Phantom;

extern "C"
{

#include <genode_disk_private.h>

    void driver_genode_update_disk_dev(genode_disk_dev_t *dest)
    {
        auto info = main_obj->_disk.getInfo();
        static char name[] = "GenodeDisk0";

        dest->block_count = info.block_count;
        dest->block_size = info.block_size;
        dest->name = name;
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