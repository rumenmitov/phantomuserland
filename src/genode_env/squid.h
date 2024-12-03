/**
 * @Author Rumen Mitov
 * @Date 2024-09-07

 squid.h provides an API to snapshot data to the disk.
 It is organized as a Radix Tree with an Upper Directory (L1), and a
 Lower Directory (L2).

 L1         - determined by the first two chars of the hash.
 L2         - determined by the next pair of chars of the hash.
 Squid File - determined by remaining chars in hash.
*/

#ifndef __SQUID_H
#define __SQUID_H

#ifdef __cplusplus

#include <base/component.h>
#include <base/sleep.h>
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <os/vfs.h>
#include <vfs/file_system_factory.h>
#include <base/buffered_output.h>


namespace Squid_snapshot {
    using namespace Genode;

    /**
     * @brief Error types associated with Squid Cache.
     */
    enum Error {
	OutOfHashes,
	WriteFile,
	ReadFile,
	CreateFile,
	CorruptedFile,
	None
    };

    static const unsigned int ROOT_SIZE = 16;
    static const unsigned int L1_SIZE 	= 256;
    static const unsigned int L2_SIZE 	= 1000;
    
    static const unsigned int HASH_LEN = 32;


    struct SquidFileHash 
    {
	unsigned int l1_dir;
	unsigned int l2_dir;
	unsigned int file_id;

	class L2_Dir *parent;

	SquidFileHash(unsigned int, unsigned int, unsigned int);	
	~SquidFileHash(void);
	
	Genode::Directory::Path to_path(void);
    };


    class L2_Dir 
    {
    private:
	unsigned int capacity;
	SquidFileHash *freelist;
	unsigned int freecount;

	unsigned int l1_dir;
	unsigned int l2_dir;

	class L1_Dir *parent;

    public:
	L2_Dir(unsigned int l1, unsigned int l2, unsigned int capacity = 1000);
	~L2_Dir(void);

	Genode::Directory::Path to_path(void);
	bool is_full(void);

	SquidFileHash* get_entry(void);
	void return_entry(void);
    };
    
    
    class L1_Dir 
    {
    private:
	unsigned int capacity;
	L2_Dir *freelist;
	unsigned int freecount;

	unsigned int l1_dir;

	class Squid_Root *parent;

    public:
	L1_Dir(unsigned int l1, unsigned int capacity = 1000);
	~L1_Dir(void);

	Genode::Directory::Path to_path(void);
	bool is_full(void);

	L2_Dir* get_entry(void);
	void return_entry(void);
    };
	
    
    class Squid_Root
    {
    protected:
	unsigned int capacity;
	L1_Dir *freelist;
	unsigned int freecount;

    public:
	Squid_Root(unsigned int capacity = 1000);
	~Squid_Root(void);

	Genode::Directory::Path to_path(void);
	bool is_full(void);

	L1_Dir* get_entry(void);
	void return_entry(void);

	SquidFileHash* get_hash(void);
    };


    struct Main
    {
	Env &_env;
	Main(Env &env);
	void init(void);

	Heap _heap { _env.ram(), _env.rm() };
	Attached_rom_dataspace _config { _env, "config" };
	Vfs::Global_file_system_factory _fs_factory { _heap };
	Vfs::Simple_env _vfs_env { _env, _heap, _config.xml().sub_node("vfs") };
	typedef Directory::Path Path;
	Directory _root_dir { _vfs_env };
	

	Genode::uint64_t last_snapshot = 0;

	Squid_Root *root_manager;


	/**
         * @brief Writes payload to file (creates one if it does not exist).
         */	
	enum Error _write(Path const &path, void *payload, size_t size);

	
       /**
        * @brief Reads from squid file into payload buffer.
	*/
	enum Error _read(Path const &path, void *payload);


	/**
	 * @brief Unit test.
	 */
	enum Error _test(void);

    };

    extern Main *global_squid;
};


#endif // __cplusplus

#endif // __SQUID_H
