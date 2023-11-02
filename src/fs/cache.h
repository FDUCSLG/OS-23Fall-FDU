#pragma once
#include <common/list.h>
#include <common/sem.h>
#include <fs/block_device.h>
#include <fs/defines.h>

/**
    @brief maximum number of distinct blocks that one atomic operation can hold.
 */
#define OP_MAX_NUM_BLOCKS 10

/**
    @brief the threshold of block cache to start eviction.

    if the number of cached blocks is no less than this threshold, we can
    evict some blocks in `acquire` to keep block cache small.
 */
#define EVICTION_THRESHOLD 20

/**
    @brief a block in block cache.

    @note you can add any member to this struct as you want.
 */
typedef struct {
    /**
        @brief the corresponding block number on disk.

        @note should be protected by the global lock of the block cache.

        @note required by our test. Do NOT remove it.
     */
    usize block_no;

    /**
        @brief list this block into a linked list.

        @note should be protected by the global lock of the block cache.
     */
    ListNode node;

    /**
        @brief is the block already acquired by some thread or process?

        @note should be protected by the global lock of the block cache.
     */
    bool acquired;

    /**
        @brief is the block pinned?

        A pinned block should not be evicted from the cache.

        e.g. it is dirty.

        @note should be protected by the global lock of the block cache.
     */
    bool pinned;

    /**
        @brief the sleep lock protecting `valid` and `data`.
     */
    SleepLock lock;

    /**
        @brief is the content of block loaded from disk?

        You may find it useless and it *is*. It is just a test flag read
        by our test. In your code, you should:

        * set `valid` to `false` when you allocate a new `Block` struct.
        * set `valid` to `true` only after you load the content of block from
       disk.

        @note required by our test. Do NOT remove it.
     */
    bool valid;
    /**
        @brief the real in-memory content of the block on disk.
     */
    u8 data[BLOCK_SIZE];
} Block;

/**
    @brief an atomic operation context.

    @note add any member to this struct as you want.

    @see begin_op, end_op
 */
typedef struct {
    /**
        @brief how many operation remains in this atomic operation?

        If `rm` is 0, any **new** `sync` will panic.
     */
    usize rm;
    /**
        @brief a timestamp (i.e. an ID) to identify this atomic operation.

        @note your implementation does NOT have to use this field, just ignoring
       it is OK too.

        @note only required by our test. Do NOT remove it.
     */
    usize ts;
} OpContext;

/**
    @brief interface for block caches.

    Here are some examples of using block caches as a normal caller:

    ## Read a block

    ```c
    Block *block = bcache->acquire(block_no);

    // ... read block->data here ...

    bcache->release(block);
    ```

    ## Write a block

    ```c
    // define an atomic operation context
    OpContext ctx;

    // begin an atomic operation
    bcache->begin_op(&ctx);
    Block *block = bcache->acquire(block_no);

    // ... modify block->data here ...

    // notify the block cache that "I have modified block->data"
    bcache->sync(&ctx, block);
    bcache->release(block);
    // end the atomic operation
    bcache->end_op(&ctx);
    ```
 */
typedef struct {
    /**
        @return the number of cached blocks at this moment.

        @note only required by our test to print statistics.
     */
    usize (*get_num_cached_blocks)();

    /**
        @brief declare a block as acquired by the caller.

        It reads the content of block at `block_no` from disk, and locks the
       block so that the caller can exclusively modify it.

        @return the pointer to the locked block.

        @see `release` - the counterpart of this function.
     */
    Block *(*acquire)(usize block_no);

    /**
        @brief declare an acquired block as released by the caller.

        It unlocks the block so that other threads can acquire it again.

        @note it does not need to write the block content back to disk.
     */
    void (*release)(Block *block);

    // # NOTES FOR ATOMIC OPERATIONS
    //
    // atomic operation has three states:
    // * running: this atomic operation may have more modifications.
    // * committed: this atomic operation is ended. No more modifications.
    // * checkpointed: all modifications have been already persisted to disk.
    //
    // `begin_op` creates a new running atomic operation.
    // `end_op` commits an atomic operation, and waits for it to be
    // checkpointed.

    /**
        @brief begin a new atomic operation and initialize `ctx`.

        If there are too many running operations (i.e. our logging is
        too small to hold all of them), `begin_op` should sleep until
        we can start a new operation.

        @param[out] ctx the context to be initialized.

        @throw panic if `ctx` is NULL.

        @see `end_op` - the counterpart of this function.
     */
    void (*begin_op)(OpContext *ctx);

    /**
        @brief synchronize the content of `block` to disk.

        If `ctx` is NULL, it immediately writes the content of `block` to disk.

        However this is very dangerous, since it may break atomicity of
        concurrent atomic operations. YOU SHOULD USE THIS MODE WITH CARE.

        @param ctx the atomic operation context to which this block belongs.

        @note the caller must hold the lock of `block`.

        @throw panic if the number of blocks associated with `ctx` is larger
                than `OP_MAX_NUM_BLOCKS` after `sync`
     */
    void (*sync)(OpContext *ctx, Block *block);

    /**
        @brief end the atomic operation managed by `ctx`.

        It sleeps until all associated blocks are written to disk.

        @param ctx the atomic operation context to be ended.

        @throw panic if `ctx` is NULL.
     */
    void (*end_op)(OpContext *ctx);

    // # NOTES FOR BITMAP
    //
    // every block on disk has a bit in bitmap, including blocks inside bitmap!
    //
    // usually, MBR block, super block, inode blocks, log blocks and bitmap
    // blocks are preallocated on disk, i.e. those bits for them are already set
    // in bitmap. therefore when we allocate a new block, it usually returns a
    // data block. however, nobody can prevent you freeing a non-data block :)

    /**
        @brief allocate a new zero-initialized block.

        It searches bitmap for a free block, mark it allocated and
        returns the block number.

        @param ctx since this function may write on-disk bitmap, it must be
                   associated with an atomic operation.
                   The caller must ensure that `ctx` is **running**.

        @return the block number of the allocated block.

        @note you should use `acquire`, `sync` and `release` to do disk I/O
                here.

        @throw panic if there is no free block on disk.
     */
    usize (*alloc)(OpContext *ctx);

    /**
        @brief free the block at `block_no` in bitmap.

        It will NOT panic if `block_no` is already free or invalid.

        @param ctx since this function may write on-disk bitmap, it must be
                   associated with an atomic operation.
                   The caller must ensure that `ctx` is **running**.
        @param block_no the block number to be freed.

        @note you should use `acquire`, `sync` and `release` to do disk I/O
                here.
     */
    void (*free)(OpContext *ctx, usize block_no);
} BlockCache;

/**
    @brief the global block cache instance.
 */
extern BlockCache bcache;

/**
    @brief initialize the block cache.

    This method is also responsible for restoring logs after system crash,

    i.e. it should read the uncommitted blocks from log section and
    write them back to their original positions.

    @param sblock the loaded super block.
    @param device the initialized block device.

    @note You may want to put it into `*_init` method groups.
 */
void init_bcache(const SuperBlock *sblock, const BlockDevice *device);