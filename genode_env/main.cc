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

#include "test_threads.h"

Phantom::Main *Phantom::main_obj = nullptr;

namespace Phantom
{
	void test_obj_space();
	void test_block_device();
};

void Phantom::test_obj_space()
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
}

// Block test function

void Phantom::test_block_device(Phantom::Disk_backend &disk)
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
}

void setup_adapters(Libc::Env &env)
{
	static Phantom::Main main(env);
	Phantom::main_obj = &main;

	log("Start obj_space test!");
	Phantom::test_obj_space();
	log("Finished obj_space test!");
	log("Starting block device test!");
	Phantom::test_block_device(main._disk);
	log("Finished block device test!");
}

// extern "C" void wait_for_continue(void);

void Libc::Component::construct(Libc::Env &env)
{

	Libc::with_libc([&]()
					{
		log("--- Phantom env test ---");

		log("Waiting for continue");
		// wait_for_continue();
		log("GO");

		{
			/*
			* Setup Main object
			*/
			try
			{
				setup_adapters(env);
			}
			catch (Genode::Service_denied)
			{
				error("opening block session was denied!");
			}
		}

		log("--- finished Phantom env test ---");

		env.exec_static_constructors(); });

	Libc::with_libc([]()
					{
		int p_argc = 1;
		char **p_argv = nullptr;
		char **p_envp = nullptr;
		phantom_main_entry_point(p_argc, p_argv, p_envp); });
}

int main()
{
	log("What are we doing here???");
}