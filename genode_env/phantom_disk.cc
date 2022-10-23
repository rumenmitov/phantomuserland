#include <base/log.h>
// #include <stdio.h>
// #include <stddef.h>
// #include <stdlib.h>
#include "phantom_env.h"

using namespace Phantom;

extern "C"
{

#include <genode_disk_private.h>

    // XXX : Phantom expects this size of the block. Need to be fixed in the future
    static int block_size = 512;

    void driver_genode_update_disk_dev(genode_disk_dev_t *dest)
    {
        auto info = main_obj->_disk.getInfo();
        static char name[] = "GenodeDisk0";

        dest->block_count = info.block_count;
        dest->block_size = info.block_size;
        dest->name = name;
    }

    // Note: block_no and len should be in blocks!
    void driver_genode_disk_read(genode_disk_dev_t *vd, void *dest, int block_no, size_t len)
    {
        (void)vd;

        if (len % block_size != 0)
        {
            Genode::warning("Incorrect len for disk read, not block aligned! (addr=", dest, " block=", block_no, " len=", len, ")");
            return;
        }

        // TODO : Do we need to sync here?
        main_obj->_disk.submit(Disk_backend::Operation::READ, false, block_no * block_size, len, dest);
    }

    void driver_genode_disk_write(genode_disk_dev_t *vd, void *src, int block_no, size_t len)
    {
        (void)vd;

        if (len % block_size != 0)
        {
            Genode::warning("Incorrect len for disk write, not block aligned! (addr=", src, " block=", block_no, " len=", len, ")");
            return;
        }

        // TODO : Do we need to sync here?
        main_obj->_disk.submit(Disk_backend::Operation::WRITE, true, block_no * block_size, len, src);
    }
}