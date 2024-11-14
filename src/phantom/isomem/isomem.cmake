set (PHANTOM_ISOMEM_DIR ${CMAKE_CURRENT_LIST_DIR})

set (source_all
    khash.c 
    udp_json.c 
    vm_cn_stats.c 
    genode_sync.c 
    wired_spin.c 
    pool.c 
    genode_unsorted.c 
    unix_hal.c 
    temp_loader_misc.c 
    test_video.c 
    test_rectangles.c 
    test_ports.c 
    test_mem.c 
    test_threads.c 
    test_dpc.c 
    test_timed_call.c 
    test_ps2_mouse.c 
    test_vm_map.c 
    hexdump.c 
    genode_credentials.c 
    genode_timer.c 
    vm_connect.c 
    genode_vmem.c 
    test_switch.c 
    vm_load.c 
    fs_map.c 
    vm_map.c 
    hal_interrupts.c 
    vm_cn_fio.c 
    dpc.c 
    test_hdir.c 
    json_read.c 
    boot_cmd_line.c 
    genode_lprintf.c 
    main.c 
    genode_atomic.c 
    disk_struct.c 
    hashfunc.c 
    timedcall.c 
    genode_physalloc.c 
    pager.c 
    ide_io.c 
    genode_user_trap.c 
    hal.c 
    genode_threads.c 
    fsck.c 
    net_timer.c 
    unix_sys.c 
    disk.c 
    genode_disk.c 
    json_write.c 
    color.c 
    test_misc.c 
    pagelist.c 
    debug_misc.c 
    driver_pci_intel_etc.c 
    genode_kunix.c 
    test_pool.c 
    vm_cn_url.c 
    genode_misc.c 
    disk_pool.c 
    physalloc.c 
    snap_sync.c 
    stats.c 
    genode_arch.c 
    syscalls_misc.c 
    time.c 
    test_disk.c 
    temp_wtty.c 
    vm_cn_timer.c 
    genode_time.c 
    amap.c 
    init_funcs.c 
    hal_physmem.c 
    test_amap.c 
    genode_gdb_cmd_stub.c 
    genode_net.c 
    genode_intrdisp.c
    init.c 
    vm_cn_udp.c
    contrib/bcd.c 
    contrib/index.c 
    contrib/rindex.c 
    contrib/strcat.c 
    contrib/strchr.c 
    contrib/strcmp.c 
    contrib/strcpy.c 
    contrib/strdup.c 
    contrib/strlcat.c 
    contrib/strlcpy.c 
    contrib/strlen.c 
    contrib/strncat.c 
    contrib/strncmp.c 
    contrib/strncpy.c 
    contrib/strnlen.c 
    contrib/strrchr.c 
    contrib/strstr.c
    contrib/memchr.c
    contrib/subr_prf.c 
    contrib/subr_scanf.c
)

if (NOT PHANTOM_BUILD_NO_DISPLAY)
  set(source_all ${source_all} genode_framebuffer.c)
endif ()

# convert filenames to absolute paths
list(TRANSFORM source_all PREPEND ${PHANTOM_ISOMEM_DIR}/)

set (PHANTOM_ISOMEM_SOURCE ${source_all})
