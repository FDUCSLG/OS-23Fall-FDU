#include "driver/uart.h"
#include "kernel/init.h"
#include "kernel/printk.h"
#include <aarch64/intrinsic.h>
#include <common/string.h>
#include <kernel/init.h>
#include <driver/memlayout.h>
#include <aarch64/mmu.h>
#include <kernel/mem.h>

static bool boot_secondary_cpus = false;

NO_RETURN void idle_entry();

void kernel_init()
{
    extern char edata[], end[];
    memset(edata, 0, (usize)(end - edata));
    do_early_init();
    do_init();
    boot_secondary_cpus = true;
}


void main()
{
    if (cpuid() == 0)
    {
        kernel_init();
        // printk("%d\n", (int)(sizeof(mem_block)));
    }
    else
    {
        while (!boot_secondary_cpus);
        arch_dsb_sy();
    }

    // enter idle process
    set_return_addr(idle_entry);
}
