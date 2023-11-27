#pragma once

#include <aarch64/mmu.h>
#include <common/defines.h>
#include <common/list.h>
#include <common/rc.h>

struct page {
    RefCount ref;
};

WARN_RESULT void *get_zero_page();

WARN_RESULT void *kalloc_page();
void kfree_page(void *);

WARN_RESULT void *kalloc(isize);
void kfree(void *);