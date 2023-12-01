#include "aarch64/intrinsic.h"
#include "kernel/printk.h"
#include <aarch64/mmu.h>
#include <common/checker.h>
#include <common/defines.h>
#include <common/list.h>
#include <common/rc.h>
#include <common/spinlock.h>
#include <driver/memlayout.h>
#include <kernel/init.h>
#include <kernel/mem.h>

#define K_DEBUG 0

#define FAIL(...)                                                              \
    {                                                                          \
        printk(__VA_ARGS__);                                                   \
        while (1)                                                              \
            ;                                                                  \
    }

RefCount alloc_page_cnt;

define_early_init(alloc_page_cnt) { init_rc(&alloc_page_cnt); }

static QueueNode *pages;
extern char end[];
define_early_init(pages) {
    for (u64 p = PAGE_BASE((u64)&end) + PAGE_SIZE; p < P2K(PHYSTOP);
         p += PAGE_SIZE)
        add_to_queue(&pages, (QueueNode *)p);
}

void *kalloc_page() {
    _increment_rc(&alloc_page_cnt);
    // TODO
    return fetch_from_queue(&pages);
}

void kfree_page(void *p) {
    _decrement_rc(&alloc_page_cnt);
    // TODO
    add_to_queue(&pages, (QueueNode *)p);
}

// TODO: kalloc kfree

u64 left_page_cnt() { return PAGE_COUNT - alloc_page_cnt.count; }