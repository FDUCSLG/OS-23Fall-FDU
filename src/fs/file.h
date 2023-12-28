#pragma once

#include <common/defines.h>
#include <common/sem.h>
#include <fs/defines.h>
#include <fs/fs.h>
#include <fs/inode.h>
#include <sys/stat.h>
#include <common/list.h>

// maximum number of open files in the whole system.
#define NFILE 65536  

typedef struct file {
    // type of the file.
    // Note that a device file will be FD_INODE too.
    enum { FD_NONE, FD_PIPE, FD_INODE } type;
    // reference count.
    int ref;
    // whether the file is readable or writable.
    bool readable, writable;
    // corresponding underlying object for the file.
    union {
        struct pipe* pipe;
        Inode* ip;
    };
    // offset of the file in bytes.
    // For a pipe, it is the number of bytes that have been written/read.
    usize off;
} File;

struct ftable {
    // TODO: table of file objects in the system

    // Note: you may need a lock to prevent concurrent access to the table!
};

struct oftable {
    // TODO: table of opened file descriptors in a process
};

// initialize the global file table.
void init_ftable();
// initialize the opened file table for a process.
void init_oftable(struct oftable*);

/**
    @brief find an unused (i.e. ref == 0) file in the global file table and set ref to 1.
    
    @return struct file* the found file object.
 */
struct file* file_alloc();

/**
    @brief duplicate a file object by increasing its reference count.
    
    @return struct file* the same file object.

    @see `inode_share` does the similar thing for inode.
 */
struct file* file_dup(struct file* f);

/**
    @brief decrease the reference count of a file object.

    If f->ref == 0, really close the file and put the inode (or close the pipe).

    @note since `cache.end_op` may sleep, you should not hold any lock (I mean, the lock for `ftable`)
    when calling `end_op`! Before you put the inode, release the lock first.

    @see `inode_put` does the similar thing for inode.
 */
void file_close(struct file* f);

/**
    @brief read the metadata of a file.

    You do not need to completely implement this method by yourself. Just call `stati`.
    
    @param[out] st the stat struct to be filled.
    @return int 0 on success, or -1 on error.

    @see `stati` will fill `st` for an inode.
 */
int file_stat(struct file* f, struct stat* st);

/**
    @brief read the content of `f` with range [f->off, f->off + n).

    
    @param[out] addr the buffer to be filled.
    @param n the number of bytes to read.
    @return isize the number of bytes actually read. -1 on error.
 */
isize file_read(struct file* f, char* addr, isize n);

/**
    @brief write the content of `f` with range [f->off, f->off + n).

    @param addr the buffer to be written.
    @param n the number of bytes to write.
    @return isize the number of bytes actually written. -1 on error.
*/
isize file_write(struct file* f, char* addr, isize n);