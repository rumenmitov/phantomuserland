#ifndef __SQUID_CACHE_H
#define __SQUID_CACHE_H

#include "vm_map.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

/*
 * Squid Structure is as follows:
 * Upper Directory - designated by first two chars of hash
 * Middle Directory - designated by second pair chars of hash
 * Lower Directory - designated by third pair chars of hash
 * Squid files - files that contain virtual page information
 */

#define PAGE_SIZE INT_MAX
#define MD5_HASH_LEN 32


void squid_generate_hash(squid_t *sq) {
    /*
     * NOTE:
     * This hashing is only a temporary solution. 
     * In the future replace with either MD5 or sha512.
     */
    memset(sq->id, 0, MD5_HASH_LEN);
    
    unsigned long timestamp = time(NULL);
    char* timestamp_str;
    snprintf(timestamp_str, MD5_HASH_LEN, "%lu", timestamp);
    
    strncpy(sq->id, timestamp_str, strlen(timestamp_str));
}




typedef struct squid {
    char id[MD5_HASH_LEN];
    char *disk_path;
} squid_t;


/* 
 * @brief Retrieves page based on its squid.
 * @param squid_t *sf
 * @returns vm_page*
 */
vm_page* squid_find(squid_t *sq) {
    FILE *fd = fopen(sq->disk_path, "rb");
    if (fd == NULL) {
        fclose(fd);
        return NULL;
    }

    vm_page* me = NULL;
    if ( fread(me, PAGE_SIZE, 1, fd) <= 0) {
        fclose(fd);
        return NULL;
    }

    fclose(fd);
    return me;
}


/*
 * @brief Generates and inserts squid based on virtual page.
 * @param vm_page *me
 * @param squid_t *sq
 * @returns int
 */
int squid_insert(squid_t *sq, vm_page *me) {
    char *upper, *middle, *lower, *file, *full_path;
    strncpy(sq->id, upper, 2);
    strncpy(sq->id + 2, middle, 2);
    strncpy(sq->id + 4, lower, 2);
    strncpy(sq->id + 6, file, MD5_HASH_LEN - 6);

    snprintf(full_path, 2 * MD5_HASH_LEN, "/squid-cache/%s/%s/%s/%s.squid", 
            upper,
            middle,
            lower,
            file);

    FILE *fd = fopen(full_path, "wb");
    if (fd == NULL) {
        fclose(fd);
        return -1;
    }

    if ( fwrite(me, PAGE_SIZE, 1, fd) <= 0 ) {
        fclose(fd);
        return -1;
    }

    fclose(fd);

    strncpy(sq->disk_path, full_path, 2 * MD5_HASH_LEN);
    return 0;
}


/*
 * @brief Deletes the squid.
 * @param squid_t *sq
 * @returns int
 */
int squid_delete(squid_t *sq) {
    int res = remove(sq->disk_path);
    free(sq->disk_path);
    return res;
}


#endif // __SQUID_CACHE_H
