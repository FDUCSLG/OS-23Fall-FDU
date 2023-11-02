#pragma once

#include <fs/defines.h>

/**
    @brief interface for block devices.

    @note yes, there is no OOP in C, but we can use function pointers to
   simulate it. this is a common pattern in C, and you can find it in Linux
   kernel too.

    @see init_block_device
 */
typedef struct {
    /**
        read `BLOCK_SIZE` bytes in block at `block_no` to `buffer`.
        caller must guarantee `buffer` is large enough.

        @param[in] block_no the block number to read from.
        @param[out] buffer the buffer to read into.
     */
    void (*read)(usize block_no, u8 *buffer);

    /**
        write `BLOCK_SIZE` bytes in `buffer` to block at `block_no`.
        caller must guarantee `buffer` is large enough.

        @param[in] block_no the block number to write to.
        @param[in] buffer the buffer to write from.
     */
    void (*write)(usize block_no, u8 *buffer);
} BlockDevice;

/**
    @brief the global block device instance.
 */
extern BlockDevice block_device;

/**
    @brief initialize the block device.

    This method must be called before any other block device methods,
    and initializes the global block device and (if necessary) the
    global super block.

    e.g. for the SD card, this method is responsible for initializing
    the SD card and reading the super block from the SD card.

    @note You may want to put it into `*_init` method groups.
 */
void init_block_device();

/**
 * @brief get the global super block.
 *
 * @return const SuperBlock* the global super block.
 */
const SuperBlock *get_super_block();