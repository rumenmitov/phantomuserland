#include <squidlib.h>

#include <ph_io.h>
#include "squid.h"
#include "phantom_timer.h"


namespace Squid_snapshot {

    L2_Dir::L2_Dir(void) 
    {
	for (unsigned int i = 0; i < CAPACITY; i++) {
	    this->available_arr[i] = i;
	}
    }


    unsigned int L2_Dir::get_hash(void) 
    {
	// TODO improve error handling when max cap is reached
	// currently hash 0 is not used
	if (this->available_count == 0) return 0;

	this->available_count--;
	return this->available_arr[this->available_count];
    }


    void L2_Dir::return_hash(unsigned int hash)
    {
	if (this->available_count == CAPACITY) return;

	this->available_count++;
	this->available_arr[this->available_count] = hash;
    }
    
    
    SquidFileHash::SquidFileHash(void)
	: l1_dir(0), l2_dir(0), file_id(0) {}

    
    SquidFileHash::SquidFileHash(L2_Dir availability_matrix[16][256])
	: l1_dir(0), l2_dir(0), file_id(0)
    {
	for (unsigned int i = 0; i < Squid_snapshot::L1_SIZE; i++) {
	    for (unsigned int j = 0; j < Squid_snapshot::L2_SIZE; j++) {
		this->file_id = availability_matrix[i][j].get_hash();

		if (this->file_id != 0) return;
	    }
	}

	Genode::error("SQUID ERROR: No more hashes available!");
    }

    SquidFileHash::~SquidFileHash(void) 
    {
	// TODO error checking
	Squid_snapshot::global_squid->_root_dir.unlink(this->to_path());
	Squid_snapshot::global_squid->
	    availability_matrix[this->l1_dir][this->l2_dir]
	    .return_hash(this->file_id);
    }
    


    Genode::Directory::Path SquidFileHash::to_path(void) 
    {
	auto hash_res =
	    Squid_snapshot::global_squid->_heap.try_alloc( sizeof(char) * Squid_snapshot::HASH_LEN );
	
	char *hash = nullptr;
	
	hash_res.with_result(
	    [&](void *addr) { hash = (char*) addr; },
	    [&](Genode::Allocator::Alloc_error err) { Genode::error(err); });

	if (hash == nullptr) Genode::error("memory allocation nullptr");

	ph_snprintf(hash, Squid_snapshot::HASH_LEN, "/squid-cache/%llu, %x/%x/%x",
		    Squid_snapshot::global_squid->last_snapshot,
		    l1_dir,
		    l2_dir,
		    file_id);

	return Cstring(hash);
    }    


    Main::Main(Env &env) : _env(env) 
    {
	if (last_snapshot == 0) {
	    Phantom::Timer_adapter timer { _env = env };
	    last_snapshot =  timer.curr_time_us();
	}
	
	for (unsigned int i = 0; i < Squid_snapshot::L1_SIZE; i++) {
	    char l1_dir[150];
	    Genode::memset(l1_dir, 0, 150);
	    ph_snprintf(l1_dir, 150, "/squid-cache/%llu/%x", last_snapshot, i);
	    
	    _root_dir.create_sub_directory(l1_dir);
	    if (!_root_dir.directory_exists(l1_dir)) {
		error("ERROR: couldn't create directory");
	    }

	    for (unsigned int j = 0; j < Squid_snapshot::L2_SIZE; j++) {
		char l2_dir[150];
		Genode::memset(l2_dir, 0, 150);
		ph_snprintf(l2_dir, 150, "/squid-cache/%llu/%x/%x", last_snapshot, i, j);
	    
		_root_dir.create_sub_directory(l2_dir);
		if (!_root_dir.directory_exists(l2_dir)) {
		    error("ERROR: couldn't create directory");
		}
	    }
	}
    }


    Error Main::_write(Path const &path, void *payload, size_t size) 
    {
	try {
	    New_file file(_root_dir, path);

	    if ( file.append((const char *) payload, size) != New_file::Append_result::OK )
		return Error::WriteFile;
		  
	} catch (New_file::Create_failed) {
	    return Error::CreateFile;
	}

	return Error::None;
    }

	  
    Error Main::_read(Path const &path, void *payload) 
    {
	Readonly_file file(_root_dir, path);
	Readonly_file::At at { 0 };

	Byte_range_ptr buffer((char*)payload, 1024);

	try {
	    for (;;) {

		size_t const read_bytes = file.read(at, buffer);

		at.value += read_bytes;

		if (read_bytes < buffer.num_bytes)
		    break;
	    }

	} catch (...) {
	    return Error::ReadFile;
	}

	return Error::None;
    }


    Error Main::_test(void) 
    {
	char message[] = "payload";
	SquidFileHash hash(global_squid->availability_matrix);
	
	switch (Main::_write(hash.to_path(), (void*) message, sizeof(message) / sizeof(char))) {
	case Error::CreateFile:
	    return Error::CreateFile;

	case Error::WriteFile:
	    return Error::WriteFile;

	default:
	    break;
	}
	

	auto echo_res = Squid_snapshot::global_squid->_heap.try_alloc( sizeof(char) * 20 );
	char *echo = nullptr;
	
	echo_res.with_result(
	    [&](void *addr) { echo = (char*) addr; },
	    [&](Genode::Allocator::Alloc_error err) { Genode::error(err); });

	if (echo == nullptr) Genode::error("memory allocation nullptr");
	
	switch (Squid_snapshot::global_squid->_read(hash.to_path(), (void*) echo)) {
	case Error::ReadFile:
	    return Error::ReadFile;

	default:
	    break;
	}

	char *i = echo;
	char *j = message;

	while (*i != 0 && *j != 0) {
	    if (*i != *j) return Error::CorruptedFile;

	    i++;
	    j++;
	}

	if (*i != 0 || *j != 0) return Error::CorruptedFile;

	return Error::None;
    }
};


#ifdef __cplusplus

extern "C" {

    #include <squidlib.h>
    
    void squid_hash(void *hash) 
    {
	// #warning should handle fail case (no more hashes)
	Squid_snapshot::SquidFileHash squid_generated_hash(Squid_snapshot::global_squid->availability_matrix);
	*((Squid_snapshot::SquidFileHash*) hash) = squid_generated_hash;
    }


    enum SquidError squid_write(void *hash, void *payload, unsigned long long size) 
    {
	auto path = ((Squid_snapshot::SquidFileHash*) hash)->to_path();
	
	switch (Squid_snapshot::global_squid->_write(path, payload, size)) {
	case Squid_snapshot::Error::CreateFile:
	    return SQUID_CREATE;

	case Squid_snapshot::Error::WriteFile:
	    return SQUID_WRITE;

	default:
	    return SQUID_NONE;
	}
    }

    
    enum SquidError squid_read(void *hash, void *payload) 
    {
	auto path = ((Squid_snapshot::SquidFileHash*) hash)->to_path();
	
	switch ( Squid_snapshot::global_squid->_read(path, payload) ) {
	case Squid_snapshot::Error::ReadFile:
	    return SQUID_READ;

	default:
	    return SQUID_NONE;
	}
    }
    
    enum SquidError squid_delete(void *hash) 
    {
	Squid_snapshot::SquidFileHash* file = (Squid_snapshot::SquidFileHash*) hash;

	try {
	    delete file;
	} catch (...) {
	    return SQUID_DELETE;
	}
	
	return SQUID_NONE;
    }

    enum SquidError squid_test(void) 
    {
	switch (Squid_snapshot::global_squid->_test()) {
	case Squid_snapshot::Error::CreateFile:
	    return SQUID_CREATE;

	case Squid_snapshot::Error::WriteFile:
	    return SQUID_WRITE;

	case Squid_snapshot::Error::ReadFile:
	    return SQUID_READ;

	case Squid_snapshot::Error::CorruptedFile:
	    return SQUID_CORRUPTED;

	default:
	    return SQUID_NONE;
	}
    }
}

#endif // __cplusplus
