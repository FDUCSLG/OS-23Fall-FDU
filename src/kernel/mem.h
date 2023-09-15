#pragma once

#include <common/defines.h>
#include <aarch64/mmu.h>
#include <common/list.h>

void* kalloc_page();
void kfree_page(void*);

void* kalloc(isize);
void kfree(void*);

