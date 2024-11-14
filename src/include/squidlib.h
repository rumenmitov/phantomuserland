/**
 * @Author Rumen Mitov
 * @Date 2024-09-07

 squid.h provides an API to snapshot data to the disk.
 It is organized as a Radix Tree with an Upper Directory, Middle Directory, and a
 Lower Directory.

 Upper      - determined by the first two chars of the hash.
 Middle     - determined by the next pair of chars of the hash.
 Lower      - determined by the third pair of chars of the hash.
 Squid File - determined by remaining chars in hash.
*/

#ifndef __SQUIDLIB_H
#define __SQUIDLIB_H


#ifdef __cplusplus
extern "C" {
#endif
    
enum SquidError {
    SQUID_WRITE,
    SQUID_CREATE,
    SQUID_READ,
    SQUID_CORRUPTED,
    SQUID_DELETE,
    SQUID_NONE
};

#define SQUID_ERROR_RED "\033[31m"
#define SQUID_ERROR_RESET "\033[0m"

#define SQUID_ERROR_FMT "[" SQUID_ERROR_RED "SQUID ERROR" SQUID_ERROR_RESET "] "


void squid_hash(void *hash);
enum SquidError squid_write(void *hash, void *payload, unsigned long long size);
enum SquidError squid_read(void *hash, void *payload);
enum SquidError squid_delete(void *hash);

enum SquidError squid_test(void);

#ifdef __cplusplus
}
#endif
    
#endif // __SQUIDLIB_H
