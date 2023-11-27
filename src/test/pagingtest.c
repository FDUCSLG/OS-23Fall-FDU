#include <common/rc.h>
#include <common/sem.h>
#include <common/string.h>
#include <kernel/mem.h>
#include <kernel/paging.h>
#include <kernel/printk.h>
#include <kernel/proc.h>
#include <kernel/pt.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <test/test.h>

bool check_zero_page() {
    for (u64 i = 0; i < PAGE_SIZE; i++)
        if (((u8 *)get_zero_page())[i] != 0)
            return false;
    return true;
}

// only need the heap section
// the heap section start at 0x0
void pgfault_first_test() {
    // init
    i64 limit = 10; // do not need too big
    struct proc *p = thisproc();
    struct pgdir *pd = &p->pgdir;
    ASSERT(pd->pt); // make sure the attached pt is valid
    attach_pgdir(pd);
    struct section *st = NULL;
    _for_in_list(node, &pd->section_head) {
        if (node == &pd->section_head)
            continue;
        st = container_of(node, struct section, stnode);
        if (st->flags & ST_HEAP)
            break;
    }
    ASSERT(st);

    // lazy allocation
    u64 pc = left_page_cnt();
    printk("in lazy allocation\n");
    for (i64 i = 1; i < limit; i++) {
        sbrk(i * PAGE_SIZE);
        ASSERT(pc == left_page_cnt());
        sbrk(-i * PAGE_SIZE);
    }
    for (i64 i = 1; i < limit; i++) {
        u64 addr = sbrk(i * PAGE_SIZE);
        *(i64 *)addr = i;
        ASSERT(*(i64 *)addr == i);
        sbrk(-i * PAGE_SIZE);
    }
    sbrk(limit * PAGE_SIZE);
    for (i64 i = 0; i < limit; i++) {
        u64 addr = (u64)i * PAGE_SIZE;
        *(i64 *)addr = i;
        ASSERT(*(i64 *)addr == i);
    }
    sbrk(-limit * PAGE_SIZE);

    // COW
    printk("in COW\n");
    pc = left_page_cnt();
    sbrk(limit * PAGE_SIZE);
    for (i64 i = 0; i < limit; ++i) {
        u64 va = i * PAGE_SIZE;
        vmmap(pd, va, get_zero_page(), PTE_RO | PTE_USER_DATA);
        ASSERT(*(i64 *)va == 0);
    }
    ASSERT(pc == left_page_cnt());
    arch_tlbi_vmalle1is(); // WHY need this line
    for (i64 i = 0; i < limit; ++i) {
        u64 va = (u64)i * PAGE_SIZE;
        *(i64 *)va = i;
    }
    if (!check_zero_page())
        PANIC();
    printk("pgfault_first_test PASS!\n");
}

void pgfault_second_test() {
    // init
    i64 limit = 10; // do not need too big
    struct pgdir *pd = &thisproc()->pgdir;
    init_pgdir(pd);
    attach_pgdir(pd);
    struct section *st = NULL;
    _for_in_list(node, &pd->section_head) {
        if (node == &pd->section_head)
            continue;
        st = container_of(node, struct section, stnode);
        if (st->flags & ST_HEAP)
            break;
    }
    ASSERT(st);

    // COW + lazy allocation
    sbrk(limit * PAGE_SIZE);
    for (i64 i = 0; i < limit / 2; ++i) {
        u64 va = i * PAGE_SIZE;
        vmmap(pd, va, get_zero_page(), PTE_RO | PTE_USER_DATA);
    }
    arch_tlbi_vmalle1is();
    for (i64 i = 0; i < limit; ++i) {
        u64 va = (u64)i * PAGE_SIZE;
        *(i64 *)va = i;
    }
    sbrk(-limit * PAGE_SIZE);
    if (!check_zero_page())
        PANIC();
    printk("pgfault_second_test PASS!\n");
}