#pragma once

#include <aarch64/mmu.h>
#include <common/defines.h>
#include <common/list.h>
#include <common/rc.h>

#define PAGE_COUNT ((P2K(PHYSTOP) - PAGE_BASE((u64) & end)) / PAGE_SIZE - 1)

struct page {
    RefCount ref;
};

u64 left_page_cnt();

WARN_RESULT void *get_zero_page();

WARN_RESULT void *kalloc_page();
void kfree_page(void *);

WARN_RESULT void *kalloc(isize);
void kfree(void *);