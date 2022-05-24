#include "tests_adapters.h"
#include "phantom_env.h"

using namespace Phantom;

bool Phantom::test_obj_space()
{

    // Reading from mem

    log("Reading from obj.space");

    addr_t read_addr = main_obj->_vmem_adapter.OBJECT_SPACE_START + 10;

    log("  read     mem                         ",
        (sizeof(void *) == 8) ? "                " : "",
        Hex_range<addr_t>(main_obj->_vmem_adapter.OBJECT_SPACE_START, main_obj->_vmem_adapter.OBJECT_SPACE_SIZE), " value=",
        Hex(*(unsigned *)(read_addr)));

    // Writing to mem

    log("Writing to obj.space");
    *((unsigned *)read_addr) = 256;

    log("    wrote    mem   ",
        Hex_range<addr_t>(main_obj->_vmem_adapter.OBJECT_SPACE_START, main_obj->_vmem_adapter.OBJECT_SPACE_SIZE), " with value=",
        Hex(*(unsigned *)read_addr));

    // Reading again

    log("Reading from obj.space");
    log("  read     mem                         ",
        (sizeof(void *) == 8) ? "                " : "",
        Hex_range<addr_t>(main_obj->_vmem_adapter.OBJECT_SPACE_START, main_obj->_vmem_adapter.OBJECT_SPACE_SIZE), " value=",
        Hex(*(unsigned *)(read_addr)));

    return true;
}

bool Phantom::test_block_device_adapter(Phantom::Disk_backend &disk)
{
    char buffer[512] = {0};
    char test_word[] = "Hello, World!";
    bool success = false;

    // Write then read

    memcpy(buffer, test_word, strlen(test_word));

    log("Writing to the disk");
    success = disk.submit(Disk_backend::Operation::WRITE, true, 1024, 512, buffer);
    log("Competed write (", success, ")");

    memset(buffer, 0x0, 512);

    log("Reading from the disk");
    success = disk.submit(Disk_backend::Operation::READ, false, 1024, 512, buffer);
    log("Competed read (", success, ")");

    log("Comparing results");
    if (strcmp(buffer, test_word) == 0)
    {
        log("Single write-read test was successfully passed!");
    }
    else
    {
        log("Single write-read test was failed!");
    }

    log("Done!");

    return true;
}

bool Phantom::test_remapping()
{
    // allocate 1 virtual and 2 physical pages

    void *v = main_obj->_vmem_adapter._obj_space_allocator.alloc(PAGE_SIZE);
    addr_t p_off = (addr_t)main_obj->_vmem_adapter.alloc_pseudo_phys(1);
    (void)p_off;
    addr_t p1 = (addr_t)main_obj->_vmem_adapter.alloc_pseudo_phys(1);
    addr_t p2 = (addr_t)main_obj->_vmem_adapter.alloc_pseudo_phys(1);

    main_obj->_vmem_adapter.map_page(p1, (addr_t)v, true);
    *(int *)v = 11;
    main_obj->_vmem_adapter.unmap_page((addr_t)v);

    main_obj->_vmem_adapter.map_page(p2, (addr_t)v, true);
    *(int *)v = 12;
    // *(int *)((char *)v + PAGE_SIZE) = 12;
    main_obj->_vmem_adapter.unmap_page((addr_t)v);

    main_obj->_vmem_adapter.map_page(p1, (addr_t)v, true);
    if (*(int *)v != 11)
    {
        error("Was not able to write 11 to phys page!");
        return false;
    }
    main_obj->_vmem_adapter.unmap_page((addr_t)v);

    main_obj->_vmem_adapter.map_page(p2, (addr_t)v, true);
    if (*(int *)v != 12)
    {
        error("Was not able to write 12 to phys page!");
        return false;
    }
    main_obj->_vmem_adapter.unmap_page((addr_t)v);

    return true;
}