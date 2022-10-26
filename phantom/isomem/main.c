


#define DEBUG_MSG_PREFIX "boot"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_info 0
#define debug_level_error 10

#include <phantom_types.h>
#include <phantom_libc.h>
#include <ph_string.h>
#include <kernel/vm.h>
#include <kernel/init.h>
#include <kernel/boot.h>
#include <kernel/page.h>

//#include <ia32/phantom_pmap.h>

#include <hal.h>
#include <multiboot.h>
#include <elf.h>
// #include <unix/uuprocess.h>



#include "misc.h"
#include <kernel/config.h>
#include <kernel/board.h>


#include <kernel/amap.h>

static amap_t ram_map;

// File specific macros

#define MEM_SIZE 0x100000000LL
#define N_OBJMEM_PAGES ((1024L*1024*32)/4096)
#define PHANTOM_VERSION_STR "0.2"

// Genode specific

#include "genode_disk.h"

// Headless video driver

struct drv_video_screen_t        *video_drv = 0;

extern int pvm_video_init(); // We need it only here

// extern void wait_for_continue();

// Init functions

void
phantom_multiboot_main()
{
    // Assuming we don't really need arch and board init. If we need, enable and implement
    /*
        arch_init_early();
        board_init_early();

        board_init_cpu_management(); // idt/gdt/ldt

        board_init_interrupts();

        arch_debug_console_init();

        // setup the floating point unit on boot CPU
        arch_float_init();
    */ 

    // Non-persistent heap initialization
    /*
        // malloc will start allocating from fixed pool.
        phantom_heap_init();
        &/
    */

    // TODO: Don't really know how amaps are used
    // amaps divide all physicall address space and apply some flags on it in order to indicate
    // for which purpose each partition is used. Probably, can be turned off
    // Initialize the memory allocator and find all available memory.
    amap_init(&ram_map, 0, MEM_SIZE, MEM_MAP_UNKNOWN );

// Memory init
// #ifdef ARCH_ia32
//     make_mem_map();
// #else
//     board_fill_memory_map(&ram_map);
// #endif

    // Suppose it to be machindep. Override if board didn't mention the kernel :)
    // extern char _start_of_kernel[], end[];
    // SET_MEM(kvtophys(_start_of_kernel), end-_start_of_kernel, MEM_MAP_KERNEL);

    // It's HAL, let's keep
    hal_init_physmem_alloc();

    // Don't need it
    // amap_iterate_all( &ram_map, process_mem_region, 0 );

    // Time to enable interrupts
    hal_sti();

    // Seem to be empty. No constructors were run
    /*
      // Call constructors.
      __phantom_run_constructors();
    */

    // Seem to be always 1 (inited on construction)
    /*
      if(!file_inited)
        SHOW_ERROR0(0, "FAIL: Constructors failed!");
      else
        SHOW_FLOW0( 7, "Constructors OK!");
    */

    // Gets information about CPU and hypervisor. Not sure if we need that
    /*
      // TODO we have 2 kinds of ia32 cpuid code
      detect_cpu(0);
      //identify_cpu();
      identify_hypervisor();
    */

    // Now time for kernel main

    // main(main_argc, main_argv, main_env);
}

// int main(int argc, char **argv, char **envp)
// {
//     phantom_main_entry_point(argc, argv, envp);
// }

int phantom_main_entry_point(int argc, char **argv, char **envp)
{
    ph_printf("Waiting...\n");
    // wait_for_continue();

    // Running adapters tests

	// if (!test_thread_creation())
	// {
	// 	ph_printf("Test creation test failed!\n");
	// 	return;
	// }

    // Running Phantom OS init routines

    phantom_multiboot_main();

    (void) envp;

    ph_snprintf( phantom_uname.machine, sizeof(phantom_uname.machine), "%s/%s", arch_name, board_name );

    // Runs all functions defined with macro INIT_ME
    run_init_functions( INIT_LEVEL_PREPARE ); // OK

    // Interrupt handlers. Probably need to be replaced later
    /*
        init_irq_allocator();
    */

    //hal_init(             (void *)PHANTOM_AMAP_START_VM_POOL,             N_OBJMEM_PAGES*4096L);

    // moved pre main()
#if 0
    // TODO we have 2 kinds of ia32 cpuid code
    detect_cpu(0);
    //identify_cpu();
    identify_hypervisor();
#endif

    phantom_paging_init(); // OK, need to be implemented

#ifdef ARCH_arm
    //test_swi();
#endif

    // OK, Inits queue and spinlock
    phantom_timed_call_init(); // Too late? Move up?
    
    // OK, to be reimplemented
    board_init_kernel_timer();

#if defined(ARCH_mips) && 0
    SHOW_FLOW0( 0, "test intr reg overflow" );
    mips_test_interrupts_integrity();
    SHOW_FLOW0( 0, "intr reg overflow test PASSED" );
#endif

    hal_init((void *)PHANTOM_AMAP_START_VM_POOL, N_OBJMEM_PAGES*4096L);

    // Threads startup
    arch_threads_init();
    // phantom_threads_init(); // Inits phantom/kernel

    // heap_init_mutex(); // After threads // OK
    pvm_alloc_threaded_init(); // After threads // OK

    // Scheduler is contolled by Genode
    /*
#if !DRIVE_SCHED_FROM_RTC // run from int 8 - rtc timer
    phantom_request_timed_call( &sched_timer, TIMEDCALL_FLAG_PERIODIC );
#endif
    hal_set_softirq_handler( SOFT_IRQ_THREADS, (void *)phantom_scheduler_soft_interrupt, 0 );
    }
    */

    //SHOW_FLOW0( 0, "Will sleep" );
    //hal_sleep_msec( 120000 );

    
    // net_timer_init(); // No need for network


    phantom_init_part_pool(); // Disk partitions. Probably needed

    // Driver stage is:
    //   0 - very early in the boot - interrupts can be used only
    //   1 - boot, most of kernel infrastructure is there
    //   2 - disks which Phantom will live in must be found here
    //   3 - late and optional and slow junk

    // We don't really need drivers here
    /*
    phantom_find_drivers( 0 );
    */

    // XXX : Find a better way to declare devices in persistent layer

    // Generally OK, but don't need to be here
    /*
    board_start_smp();
    */

    {
        extern const char* SVN_Version;
        extern struct utsname phantom_uname;
        ph_strncpy( phantom_uname.release, SVN_Version, sizeof(phantom_uname.release) );
    }

    //pressEnter("will run DPC");
    dpc_init(); // OK. DPCs are needed



    ph_printf("\n\x1b[33m\x1b[44mPhantom " PHANTOM_VERSION_STR " (SVN rev %s) @ %s starting\x1b[0m\n\n", svn_version(), phantom_uname.machine );
    phantom_process_boot_options(); // OK. Let's keep it

#if defined(ARCH_arm) && 0
    SHOW_FLOW0( 0, "test intr reg overflow" );
    arm_test_interrupts_integrity();
    SHOW_FLOW0( 0, "intr reg overflow test PASSED" );
#endif

    // GDB commands. May be ommited
    /*
    dbg_init(); // Kernel command line debugger
    */

    // Used to refill list used to allocate physmem in interrupts
    hal_init_physmem_alloc_thread();  // Ok, it is HAL

    // Network. Should be disabled for now. Should be implemented later
    /*
    phantom_port_init();
    //pressEnter("will start net");
    net_stack_init();
    */

    //pressEnter("will look for drv stage 1");
    // We don't really need drivers here
    /*
    phantom_find_drivers( 1 );
    */

    // here it kills all by calling windowing funcs sometimes
    //init_main_event_q();
    run_init_functions( INIT_LEVEL_INIT );

    //vesa3_bootstrap();

#ifdef ARCH_ia32
    SHOW_FLOW( 5, "ACPI: %d" , InitializeFullAcpi() );
#endif

#ifdef ARCH_ia32
#if HAVE_VESA
    //pressEnter("will init vm86");
    phantom_init_vm86();
    //pressEnter("will init VESA");
    if(!bootflag_no_vesa) phantom_init_vesa();
#endif
#endif
    //pressEnter("will init graphics");

    // Skip. Probably need to be implemented
    /*
    phantom_start_video_driver();
    */

    // XXX : Video initialization from pvm_headless. Should be replaced with actual video
    {
        pvm_video_init();   // Initializing headless video

        // pvm_headless workaround
        // scr_mouse_set_cursor(drv_video_get_default_mouse_bmp());

        drv_video_init_windows();
        init_main_event_q();
        init_new_windows();
    }

    //SHOW_FLOW0( 0, "Will sleep" );
    //hal_sleep_msec( 120000 );

    // vm86 and VESA die without page 0 mapped
// #if 1
// 	// unmap page 0, catch zero ptr access
// 	hal_page_control( 0, 0, page_unmap, page_noaccess );
// #endif

    // Let's skip. Probably, should be reimplemented
    /*
    init_main_event_q();
    */

    //pressEnter("will look for drv stage 2");
    // We don't really need drivers here
    /*
    phantom_find_drivers( 2 );
    */


#if HAVE_UNIX
    // Before boot modules init Unix-like environment
    // as modules have access to it
    phantom_unix_fs_init();
    phantom_unix_proc_init();

    // Start modules bootloader brought us
    phantom_start_boot_modules();
#endif // HAVE_UNIX



#if HAVE_USB
    usb_setup();
#endif // HAVE_USB

    //pressEnter("will run phantom_timed_call_init2");
    //phantom_timed_call_init2();

    // -----------------------------------------------------------------------
    // If this is test run, switch to test code
    // -----------------------------------------------------------------------
#if !defined(ARCH_arm)
    if( argc >= 3 && (0 == ph_strcmp( argv[1], "-test" )) )
    {
        SHOW_FLOW0( 0, "Sleep before tests to settle down boot activities" );
        hal_sleep_msec( 2000 );
        SHOW_FLOW( 0, "Will run '%s' test", argv[2] );
        run_test( argv[2], argv[3] );
	// CI: this message is being watched by CI scripts (ci-runtest.sh)
        SHOW_FLOW0( 0, "Test done, reboot");

        // XXX : Do proper exit!
        // exit(0);
        hal_sleep_msec(1000000);
    }
#else
    {
        (void) argv;
        (void) argc;
        SHOW_FLOW0( 0, "Will run all tests" );
        run_test( "all", "" );
	// CI: this message is being watched by CI scripts (ci-runtest.sh)
        SHOW_FLOW0( 0, "Test done, reboot");
        exit(0);
    }
#endif

    run_test( "all", "" );

#ifdef PHANTOM_TESTS_ONLY 
    ph_printf("\n\n======================\n");
    ph_printf("Testing is done!\n");
    return 0;
#endif

    // Probably not needed
    /*
#ifdef ARCH_ia32
    connect_ide_io();
#endif
    */

    // -----------------------------------------------------------------------
    // Now starting object world infrastructure
    // -----------------------------------------------------------------------


    //pressEnter("will start phantom");
    start_phantom();

#ifdef ARCH_ia32
    phantom_check_disk_check_virtmem( (void *)hal_object_space_address(), CHECKPAGES );
#endif

    SHOW_FLOW0( 2, "Will load classes module... ");
    load_classes_module();

    // We don't really need drivers here
    /*
    phantom_find_drivers( 3 );
    */


#if HAVE_NET
    phantom_tcpip_active = 1; // Tell them we finished initing network
    syslog(LOG_DEBUG|LOG_KERN, "Phantom " PHANTOM_VERSION_STR " (SVN rev %s) @ %s starting", svn_version(), phantom_uname.machine );
#endif




    SHOW_FLOW0( 2, "Will init phantom root... ");
    // Start virtual machine in special startup (single thread) mode
    pvm_root_init();

    // just test
    //phantom_smp_send_broadcast_ici();

    //pressEnter("will run vm threads");
    SHOW_FLOW0( 2, "Will run phantom threads... ");
    // Virtual machine will be run now in normal mode
    activate_all_threads();

    vm_enable_regular_snaps();

    // We don't really need drivers here
    /*
    //pressEnter("will look for drv stage 4");
    phantom_find_drivers( 4 );
    */

//trfs_testrq();

    run_init_functions( INIT_LEVEL_LATE );


    //init_wins(u_int32_t ip_addr);

    ph_printf("\n\x1b[33m\x1b[44mPhantom " PHANTOM_VERSION_STR " (SVN rev %s) @ %s started\x1b[0m\n\n", svn_version(), phantom_uname.machine );

#if 1
    {
        hal_sleep_msec(60000*13);
        ph_printf("\nWILL CRASH ON PURPOSE\n\n" );
        hal_sleep_msec(20000);
        hal_cpu_reset_real();
    }

    while(1)
        hal_sleep_msec(20000);

#else
    kernel_debugger();
#endif

    phantom_shutdown(0);
    //pressEnter("will reboot");

    panic("returned from main");
    return 0;
}

void start_phantom()
{

    ph_printf("DEBUG!!! STARTING PHANTOM\n");

    //pressEnter("will start Phantom");
    SHOW_FLOW0( 2, "Will init snap interlock... ");
    phantom_snap_threads_interlock_init();

    //pressEnter("will init paging dev");

    SHOW_FLOW0( 5, "Will init paging dev... ");

    // A: Assuming we can init all of genode stuff here
    driver_genode_disk_probe(); // <- Will init virtual dev
    driver_genode_disk_init();  // <- Will register device in Phantom and create necessary structs


#if !PAGING_PARTITION
    // TODO size?
    init_paging_device( &pdev, "wd1", 1024*20); //4096 );

    //pressEnter("will init pager");
    pager_init(&pdev);
#else
    partition_pager_init(select_phantom_partition());
#endif

    SHOW_FLOW0( 5, "Will init vm map... ");
    //pressEnter("will init VM map");
    // TODO BUG this +1 is due to allocator error:
    // allocator insists on trying to access the byte
    // just after the memory arena. Fix that and remove this +1 after.
    vm_map_init( N_OBJMEM_PAGES+1 );
}
