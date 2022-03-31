/*
 * Tests for Phantom HAL interfaces
 *
 * Those tests should be intended to run before Phantom initialization
 */

namespace Phantom
{
	bool test_hal_thread_creation();
};

extern "C" bool test_hal_vmem_alloc();
extern "C" bool test_hal_phys_alloc();
extern "C" bool test_hal_vmem_mapping();
extern "C" bool test_hal_vmem_remapping();
extern "C" bool test_hal_mutex_state();