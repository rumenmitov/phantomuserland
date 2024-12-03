#include <squidlib.h>

#include <ph_io.h>
#include "squid.h"
#include "os/vfs.h"
#include "util/string.h"
#include "phantom_env.h"


void* squid_malloc(size_t size) 
{
    size_t total_size = sizeof(size_t) + size;


    auto alloc_res = main_obj->_heap.try_alloc(total_size);

    if (!alloc_res.ok()){
        alloc_res.with_error([](Allocator::Alloc_error err){error(err);});
        return 0;
    }

    void* original_addr = nullptr;

    alloc_res.with_result(
        [&](void* addr){ original_addr = addr; }, 
        [&](Allocator::Alloc_error err){ error(err); }
    );

    // just to ensure it is safe
    if (original_addr == nullptr){
        error("ph_malloc: addr = nullptr!!!");
        return 0;
    }

    // Writing the size
    *((size_t *)original_addr) = size;

    size_t *adjusted_addr = ((size_t *)original_addr) + 1;

    return (void *)adjusted_addr;
}


void squid_free(void* addr) 
{
    if (addr == nullptr) return;

    size_t *adjusted_addr = (size_t *)addr;
    void *original_addr = (void *)(adjusted_addr - 1);

    size_t size = *((size_t *)original_addr);

    main_obj->_heap.free(addr, size);
    addr = nullptr;
}


namespace Squid_snapshot {

    Squid_Root::Squid_Root(unsigned int capacity)
	: capacity(capacity), freecount(capacity)
    {
	freelist = (L1_Dir*) squid_malloc(sizeof(L1_Dir) * capacity);

	Genode::Directory::Path path = to_path();

	global_squid->_root_dir.create_sub_directory(path);
	if (!global_squid->_root_dir.directory_exists(path)) {
	    error("ERROR: couldn't create directory: ", path);
	}

	for (unsigned int i = 0; i < capacity; i++) {
	    freelist[i] = L1_Dir(i, L1_SIZE);
	}
    }


    Squid_Root::~Squid_Root(void)
    {
	squid_free(freelist);
    }

    Genode::Directory::Path Squid_Root::to_path(void) 
    {
	return Cstring("/squid-cache/current");
    }


    bool Squid_Root::is_full(void) 
    {
	return freecount == 0;
    }
    
    
    L1_Dir* Squid_Root::get_entry(void)
    {
	if (is_full()) return nullptr;

	for (unsigned i = 0; i < capacity; i++) {
	    if (!freelist[i].is_full()) {
		freecount--;
		return &freelist[i];
	    }
	}

	error("This state should not be reacheable!");
	return nullptr;
    }


    void Squid_Root::return_entry(void) 
    {
	if (freecount == capacity) return;
	freecount++;
    }


    SquidFileHash* Squid_Root::get_hash(void) 
    {
	L1_Dir *l1 = get_entry();
	if (l1 == nullptr) return nullptr;

	L2_Dir *l2 = l1->get_entry();
	if (l2 == nullptr) return nullptr;

	return l2->get_entry();
    }
    


    L1_Dir::L1_Dir(unsigned int l1, unsigned int capacity)
	: l1_dir(l1), capacity(capacity), freecount(capacity)
    {
	freelist = (L2_Dir*) squid_malloc(sizeof(L2_Dir) * capacity);

	Genode::Directory::Path path = to_path();

	global_squid->_root_dir.create_sub_directory(path);
	if (!global_squid->_root_dir.directory_exists(path)) {
	    error("ERROR: couldn't create directory: ", path);
	}

	for (unsigned int i = 0; i < capacity; i++) {
	    freelist[i] = L2_Dir(l1_dir, i, L2_SIZE);
	}
    }


    L1_Dir::~L1_Dir(void)
    {
	squid_free(freelist);
	parent->return_entry();
    }


    Genode::Directory::Path L1_Dir::to_path(void) 
    {
	char *l1_dir_path = (char*) squid_malloc(sizeof(char) * 150);
	
	ph_snprintf(l1_dir_path, 150, "/squid-cache/current/%x", l1_dir);

	Cstring path(l1_dir_path);
	return path;
    }


    bool L1_Dir::is_full(void) 
    {
	return freecount == 0;
    }

    
    L2_Dir* L1_Dir::get_entry(void)
    {
	if (is_full()) return nullptr;

	for (unsigned i = 0; i < capacity; i++) {
	    if (!freelist[i].is_full()) {
		freecount--;
		return &freelist[i];
	    }
	}
	error("This state should not be reacheable!");
	return nullptr;
    }


    void L1_Dir::return_entry(void) 
    {
	if (freecount == capacity) return;
	freecount++;
    }
    


    L2_Dir::L2_Dir(unsigned int l1, unsigned int l2, unsigned int capacity)
	: l2_dir(l2), capacity(capacity), freecount(capacity)
    {
	freelist = (SquidFileHash*) squid_malloc(sizeof(SquidFileHash) * capacity);

	Genode::Directory::Path path = to_path();

	global_squid->_root_dir.create_sub_directory(path);
	if (!global_squid->_root_dir.directory_exists(path)) {
	    error("ERROR: couldn't create directory: ", path);
	}	

	for (unsigned int i = 0; i < capacity; i++) {
	    freelist[i] = SquidFileHash(l1_dir, l2_dir, i);
	}
    }


    L2_Dir::~L2_Dir(void)
    {
	/* NOTE
	   Used SquidFileHashes will be deleted from disk
	   when the corresponding SquidFileHash's destructor is called.
	*/
	squid_free(freelist);
	parent->return_entry();
    }


    Genode::Directory::Path L2_Dir::to_path(void) 
    {
	char *l2_dir_path = (char*) squid_malloc(sizeof(char) * 150);
	
	ph_snprintf(l2_dir_path, 150, "/squid-cache/current/%x/%x", l1_dir, l2_dir);

	Cstring path(l2_dir_path);
	
	return path;
    }


    bool L2_Dir::is_full(void) 
    {
	return freecount == 0;
    }

    
    SquidFileHash* L2_Dir::get_entry(void)
    {
	if (is_full()) return nullptr;
	
	// TODO implement as ring buffer with memory
	for (unsigned int i = 0; i < capacity; i++) {
	    Genode::Directory::Path hash = freelist[i].to_path();
	    
	    if (!global_squid->_root_dir.file_exists(hash)) {
		freecount--;

		try {
		    New_file file(global_squid->_root_dir, hash);

		    if ( file.append(0, 1) != New_file::Append_result::OK )
			throw Error::WriteFile;
		  
		} catch (New_file::Create_failed) {
		    throw Error::CreateFile;
		}

		return &freelist[i];
	    }
	}

	error("This state should not be reacheable!");
	return nullptr;
    }


    void L2_Dir::return_entry(void) 
    {
	if (freecount == capacity) return;
	freecount++;
    }


    
    SquidFileHash::SquidFileHash(unsigned int l1, unsigned int l2, unsigned int file)
	: l1_dir(l1), l2_dir(l2), file_id(file)
    {}
    
    
    SquidFileHash::~SquidFileHash(void) 
    {
	parent->return_entry();
    }
    


    Genode::Directory::Path SquidFileHash::to_path(void) 
    {
	char *hash = (char*) squid_malloc(sizeof(char) * Squid_snapshot::HASH_LEN);
	
	ph_snprintf(hash, Squid_snapshot::HASH_LEN, "/squid-cache/%x/%x/%x",
		    l1_dir,
		    l2_dir,
		    file_id);

	Cstring path(hash);
	return path;
    }    


    Main::Main(Env &env) : _env(env) 
    {}


    void Main::init(void) 
    {
	root_manager = (Squid_Root*) squid_malloc(sizeof(Squid_Root));
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

	SquidFileHash *hash = global_squid->root_manager->get_hash();
	if (hash == nullptr) return Error::OutOfHashes;
	
	switch (Main::_write(hash->to_path(), (void*) message, sizeof(message) / sizeof(char))) {
	case Error::CreateFile:
	    return Error::CreateFile;

	case Error::WriteFile:
	    return Error::WriteFile;

	default:
	    break;
	}
	

	char *echo = (char*) squid_malloc(sizeof(char) * 20);
	
	switch (Squid_snapshot::global_squid->_read(hash->to_path(), (void*) echo)) {
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
	Squid_snapshot::SquidFileHash *squid_generated_hash = Squid_snapshot::global_squid->root_manager->get_hash();
	hash = (void*) squid_generated_hash;
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
