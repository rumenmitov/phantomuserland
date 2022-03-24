/*
 * Envrionment for Phantom OS
 * Based on rm_nested example and rump block device backend
 */

#include <base/component.h>
#include <rm_session/connection.h>
#include <region_map/client.h>
#include <dataspace/client.h>

#include <base/allocator_avl.h>
#include <block_session/connection.h>

#include <libc/component.h>

#include "phantom_env.h"
#include "disk_backend.h"
#include "phantom_vmem.h"
#include "phantom_threads.h"

#include "phantom_entrypoints.h"

#include "tests_hal.h"
#include "tests_adapters.h"

Phantom::Main *Phantom::main_obj = nullptr;

void setup_adapters(Libc::Env &env)
{
	static Phantom::Main local_main(env);
	Phantom::main_obj = &local_main;
}

void test_adapters()
{
	log("Checking if main_obj is initialized");
	log(&Phantom::main_obj->_vmem_adapter);

	// log("Start obj_space test!");
	// Phantom::test_obj_space();
	// log("Finished obj_space test!");

	log("Starting remapping test!");
	test_remapping();
	log("finished remapping test!");

	log("Starting block device test!");
	Phantom::test_block_device_adapter(Phantom::main_obj->_disk);
	log("Finished block device test!");
}

bool test_hal()
{
	bool ok = true;

	// Phantom::test_hal_vmem_alloc();
	log("--- Starting vmem alloc test");
	if (!test_hal_vmem_alloc())
	{
		ok = false;
		log("Failed vmem alloc test!");
	}

	log("--- Starting phys alloc test");
	if (!test_hal_phys_alloc())
	{
		ok = false;
		log("Failed phys alloc test!");
	}

	log("--- Starting virt addrs mapping test");
	if (!test_hal_vmem_mapping())
	{
		ok = false;
		log("Failed virt addrs mapping test!");
	}

	log("--- Starting virt addrs remapping test");
	if (!test_hal_vmem_remapping())
	{
		ok = false;
		log("Failed virt addrs remapping test!");
	}

	return ok;
}

// extern "C" void wait_for_continue(void);

void Libc::Component::construct(Libc::Env &env)
{

	// Libc::with_libc([&]()
	// 				{
	// 	int p_argc = 1;
	// 	char **p_argv = nullptr;
	// 	char **p_envp = nullptr;
	// 	phantom_main_entry_point(p_argc, p_argv, p_envp); });

	// void *f_addr = 0x0;
	// log(*((int *)f_addr));

	Libc::with_libc([&]()
					{
		log("--- Phantom init ---");

		log("Waiting for continue");
		// wait_for_continue();
		log("GO");

		{
			/*
			 * Setup Main object
			 */
			try
			{
				log("--- Setting up adapters ---");
				setup_adapters(env);
			}
			catch (Genode::Service_denied)
			{
				error("opening block session was denied!");
				return;
			}
		}

		log("--- Testing adapters ---");
		test_adapters();

		log("--- Testing HAL ---");
		test_hal();

		log("--- finished Phantom env test ---");

		// env.exec_static_constructors(); <- This thing might break pthreads!

		// Actual Phantom code

		int p_argc = 1;
		char **p_argv = nullptr;
		char **p_envp = nullptr;
		phantom_main_entry_point(p_argc, p_argv, p_envp); });
}

int main()
{
	log("What are we doing here???");
}