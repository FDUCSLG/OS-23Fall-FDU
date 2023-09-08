# OS Lab API Reference（继承自去年）

<center><b>2023 年 9 月 8 日</b></center>

内核框架中提供了大量的接口，为方便同学查阅，特撰写本 API Reference。（有些接口的代码需要自行补全，请以 lab 要求为准）

API Reference 以模块顺序组织 API，正式记载的 API 以正式条目形式列出，非正式记载的 API 以简要形式列出。只有在特殊情况下才应使用非正式记载的 API。

API Reference 仅列出在 lab 中可能使用到的 API，亦不包含 API 的实现细节。如有兴趣了解未写出的内容，请联系助教。

[TOC]



# **\<aarch64/intrinsic.h\>**

## **cpuid**

返回当前的 CPUID。

**Syntax** `int cpuid()`

**Return Value** 当前的 CPUID，取值为 0、1、2、3。

## **compiler_fence**

编译器屏障，阻止编译器重排屏障前后的代码。

**Syntax** `void compiler_fence()`

## **arch_isb**

指令屏障，清空流水线。

**Syntax** `void arch_isb()`

## **arch_dsb_sy**

数据屏障，阻止处理器重排屏障前后的访存。

**Syntax** `void arch_dsb()`

## **arch_fence**

指令 & 数据屏障，相当于 isb+dsb。

**Syntax** `void arch_fence()`

## **device_put_u32**

向 MMIO 地址写入 4 字节数据。

**Syntax** `void device_put_u32(u64 addr, u32 value)`

**Parameters**

- addr 要写入数据的 MMIO 地址

- value 要写入的 4 字节数据

**Remarks** device_put_u32与直接指针写入的区别在于，device_put_u32 通过编译器屏障和volatile关键字禁止编译器对写入的优化。

## **device_get_u32**

从 MMIO 地址读取 4 字节数据。

**Syntax** `u64 device_get_u32(u64 addr)`

**Parameters**

- addr 要读取数据的 MMIO 地址

**Remarks** device_get_u32与直接指针读取的区别在于，device_get_u32 通过编译器屏障和volatile关键字禁止编译器对读取的优化。

## **arch_wfi**

暂停处理器执行，等待中断发生。

**Syntax** `void arch_wfi()`

**Remarks** 暂停时处理器处于低功耗状态（qemu 不会模拟这一点）。

## **arch_yield**

提示处理器可以适当暂停执行当前代码，常用于spinlock。

**Syntax** `void arch_yield()`

**Remarks** 一般用于支持硬件线程（Intel 称 Hyperthread 技术）的处理器， 提示处理器进行任务切换（qemu 不会模拟这一点）。

## **arch_with_trap**

默认情况下，内核不开启中断。arch_with_trap标记表示代码块在开启中断的情况下执行，常与arch_wfi配合使用。

**Syntax** `arch_with_trap { ... }`

## **delay_us**

延时若干微秒后返回，延时期间不能执行其他代码（除非开启中断）。

**Syntax** `void delay_us(u64 n)`

**Parameters**

-   n 延时的微秒数

## **set_return_addr**

修改当前函数的返回地址。

**Syntax** `#define set_return_addr(addr)`

**Parameters**

-   addr 任意可转换为u64的类型，值为要设置的返回地址。

## **(Undocumented)**

-   `bool _arch_enable_trap()` 开启中断。返回值为之前的中断开关状态。
-   `bool _arch_disable_trap()` 关闭中断。返回值为之前的中断开关状态。
-   `void arch_set_vbar(void* ptr`) 将当前 CPU 的异常向量基地址设置为指定虚拟地址。一般情况下，只应在set_cpu_on中为每个 CPU 执行一次。
-   `void arch_reset_esr()` 清零 esr 寄存器，复位异常信号。
-   `u64 get_timestamp()` 获取时间戳。单位由处理器决定。

# \<aarch64/mmu.h\>

## **K2P**

获取内核虚拟地址对应的物理地址。

**Syntax** `#define K2P(addr)`

**Parameters**

- addr 任意可转换为u64的类型，为要转换的内核虚拟地址。

**Return Value** u64类型，为对应的物理地址。

## **P2K**

获取物理地址对应的内核虚拟地址。

**Syntax** `#define P2K(addr)`

**Parameters**

- addr 任意可转换为u64的类型，为要转换的物理地址。

**Return Value** u64类型，为对应的内核虚拟地址。

## **P2N**

获取物理地址对应的物理页号。

**Syntax** `#define P2N(addr)`

**Parameters**

- addr 任意可转换为u64的类型，为要转换的物理地址。

**Return Value** u64类型，为对应的物理页号。

## **PAGE_BASE**

获取内存地址对应的页基地址。

**Syntax** `#define PAGE_BASE(addr)` 

**Parameters**

- addr 任意可转换为u64的类型，为要获取页基地址的内存地址。

**Return Value** u64类型，为对应的页基地址。如果内存地址是虚拟地址，返 回虚拟页基地址；如果内存地址是物理地址，返回物理页基地址。

## **PTE_ADDRESS**

获取页表项对应的下一级页表或物理页的物理地址。

**Syntax** `#define PTE_ADDRESS(pte)`

**Parameters**

- pte u64类型，页表项。

**Return Value** u64类型，页表项对应的下一级页表或物理页的物理地址。 这个宏只取出页表项的相应位数据，不检查页表项是否有效。无效的页表项的PTE_ADDRESS没有意义。

## **N_PTE_PER_TABLE**

每一级页表中含有的页表项数量。lab 中为 512。

## **PTE_TABLE**

指示页表项为下一级页表。

## **PTE_VALID**

指示页表项有效。

**Remarks** PTE_VALID一般用于测试页表项是否有效，在配置页表项时使用PTE_VALID是没有意义的。

## **PTE_USER_DATA**

指示页表项为用户数据页。

**Remarks** 用户数据页大小为 4K，可缓存，可在用户态读写。

# \<common/checker.h\>

## **Checker**

Checker 是配对检查器，可以与 setup_checker、checker_begin_ctx 和 checker_end_ctx 配合使用。setup 在当前的变量作用域中定义 checker，beginctx 和 endctx 分别可以为 checker 增加或减少计数。若 checker 在其生命周期（可参考 C++ 中的概念）终止时计数恒为 0（即 beginctx 和 endctx 间形成配对），则编译通过，否则会报 Checker: contextmismatching 编译错误。

一般不会单独使用配对检查器，而是将其与需要配对检查的函数组合使用，如通过宏在acquire_lock中内置 beginctx 操作，在release_lock中内置 endctx 操作，即可检查是否存在忘记放锁的情况。

可以为配对检查器注册结束回调函数。当 checker 计数归零时，会执行注册的回调函数，并清除回调函数。

以下示例代码供参考

```c
// test1 无法通过编译，qwq处于函数作用域，其生命周期在函数返回时终止
//代码显然不能保证函数返回时qwq计数恒为0（ begin_ctx 和 end_ctx 形成配对）
void test1 (int x)
{
    setup_checker (qwq) ;
    checker_begin_ctx (qwq) ;
    if(x == 0)
    	return ;
    checker_end_ctx (qwq) ;
}

// test2 无法通过编译，因为qwq处于 else 的块级作用域，生命周期在 else 块结束时终止
//else 块结束时 qwq 计数显然不为0，而且后面的 end_ctx 中 qwq 是未定义的
void test2 (int x)
{
    if(x == 0)
    	return ;
    else
    {
        setup_checker (qwq) ;
        checker_begin_ctx (qwq) ;
    }
    checker_end_ctx (qwq) ;
}

// test3 可以通过编译，虽然有些复杂，但可以看出qwq计数最终恒为0
void test3 (int x)
{
    setup_checker (qwq) ;
    checker_begin_ctx (qwq) ;
    while(x)
    {
        checker_end_ctx (qwq) ;
        if(−−x < 0)
        	return ;
        checker_begin_ctx (qwq) ;
    }
    checker_end_ctx (qwq) ;
}

// test4 将输出 ’bac’
void test4 ()
{
    setup_checker(qwq) ;
    checker_begin_ctx(qwq) ;
    checker_set_delayed_task(qwq, putch, ’a’ ) ;
    putch(’b ’) ;
    checker_end_ctx(qwq);
    checker_begin_ctx(qwq);
    putch (’c’) ;
    checker_end_ctx(qwq);
}
```

## **setup_checker**

定 义 并 初 始 化 配 对 检 查 器 。

**Syntax** `#define setup_checker(id)`

**Parameters**

-   id 任意数字、字母和下划线的组合，为自定义的检查器 ID。

## **checker_begin_ctx**

增加配对检查器计数。

**Syntax** `#define checker_begin_ctx(id)`

**Parameters**

-   id 要增加计数的检查器 ID。

## **checker_end_ctx**

减少配对检查器计数。

**Syntax** `#define checker_end_ctx(id)`

**Parameters**

- id 要减少计数的检查器 ID。

**Remarks** 当计数归零时，会执行结束回调函数，并清除结束回调函数。

## **checker_set_delayed_task**

注册结束回调函数。结束回调函数会在 checker 清零后被调用，并在调用后被清除。一次只能注册一个结束回调函数。

**Syntax** `#define checker_set_delayed_task(id, f, a)`

**Parameters**

-   id 要注册结束回调函数的检查器 ID。
-   f 结束回调函数的指针。
-   a 结束回调函数的参数。

## **(Undocumented)**

-   `checker_begin_ctx_before_call(id, f, ...)` 增加检查器计数，然后调用指定函数。返回所调用的指定函数返回值
-   `checker_end_ctx_after_call(id, f, ...)` 调用指定函数，然后减少检查器计数，会执行结束回调函数。返回所调用的指定函数返回值。

# \<common/defines.h\>

## **MIN**

计算 min。

**Syntax** `#define MIN(a, b)`

**Parameters**

- a、b为任意可比较大小的变量。

**Return Value** a、b间的较小值。

## **MAX**

计算 max。

**Syntax** `#define MAX(a, b)`

**Parameters**

- a、b为任意可比较大小的变量。

**Return Value** a、b间的较大值。

## **BIT**

计算 2 的幂。

**Syntax** `#define BIT(i)`

**Parameters**

-   i为 0-63 的整数。

**Return Value** $2^i$。

## **container_of**

通过结构体成员的指针获取结构体的指针。

**Syntax** `#define container_of(mptr, type, member)`

**Parameters**

- mptr 结构体成员的指针。

- type 结构体类型名。

- member mptr对应的结构体成员名。

**Return Value** mptr对应的type类型结构体指针。

**Remarks** 下面示例代码有助于理解

```c
struct {int a, b;};
struct c x;
int ∗y = &x.b ;
struct c ∗z = container_of(y , struct c, b); // z == &x
```

## **(Undocumented)**

-   `NO_BSS` 修饰变量。指示编译器不要将该函数放入 .bss 段。为变量赋初值也可达到一样的效果，且是更为推荐的做法。
-   `NO_RETURN` 修饰函数。提示编译器该函数不会返回，可能导致编译器采取一些更激进的优化策略。
-   `ALWAYS_INLINE` 修饰函数。提示编译器该函数可能不会被用到，并指示编译器每次用到该函数时都将其内联。
-   `NO_INLINE` 修饰函数。指示编译器不要将该函数内联。

# \<common/list.h\>

## **ListNode**

ListNode 是一个内嵌式的双向链表节点，分别使用 ListNode.prev 和 ListNode.next 进行前向和后向访问。

一般将ListNode嵌入其他数据结构中，如

```c
struct a {
	int something;
	ListNode list;
};
```

(struct a).list 连接成双向链表，可以借助提供的 container_of 得到 struct a 。

容易看出，链表不是并发安全的数据结构。因此，我们将一些链表操作的 API 设计为了需要传入一个锁的形式，以免遗忘。如需使用不带锁的版本，可参考 (Undocumented) 中带下划线的版本。

## **init_list_node**

初始化链表节点。

**Syntax** `void init_list_node(ListNode *node)`

**Parameters**

- node 要初始化的链表节点。

**Remarks** 链表节点会被初始化为自环。

## **merge_list**

将两个链表合并为一个。

**Syntax** `#define merge_list(lock, node1, node2)`

**Parameters**

- lock spinlock，函数在执行合并操作前后会自动取得和释放这个锁。

- node1、node2 要合并的两个链表指针。

**Remarks** 合并效果如下所示

```
before: (arrow is the next pointer)
... --> node1 --> node3 --> ...
... <-- node2 <-- node4 <-- ... 

after:
... --> node1 --+ +-> node3 --> ...
				+ +
... <-- node2 <-+ +-- node4 <-- ...
```

## **insert_into_list**

将节点插入链表。

**Syntax** `#define insert_into_list(lock, list, node)`

**Parameters**

- lock spinlock，函数在执行插入操作前后会自动取得和释放这个锁。

- list 被插入的链表指针。

- node 要插入的链表节点指针。无需初始化节点。

**Remarks** 节点会被插入到链表的后面，即 `list->next=node`。

## **detach_from_list**

将节点从链表中删除。

**Syntax** `#define detach_from_list(lock, node)`

**Parameters**

- lock spinlock，函数在执行删除操作前后会自动取得和释放这个锁。

- node 要删除的链表节点指针。

**Return Value** 返回被删除的链表节点前面的元素，即原先的node-\>prev。如果 node 是链表中唯一的元素，返回 NULL。

## **\_for_in_list**

一个语法糖，用于遍历链表。

**Syntax** `_for_in_list(p, list) { ... }`

**Parameters**

- p 循环变量，为链表节点的指针。

- list 指向被遍历的链表指针。

- { ... } 循环体，类似于 for，可以使用 break 和 continue。

**Remarks** 在没有 break 的情况下，遍历从 list-\>next 开始，list 结束，链表中的每个节点都会被遍历一次。**这个遍历不是并发安全的，需要手动加锁。**可以参考下面示例代码

```c
void print_list (ListNode ∗list , SpinLock ∗lock )
{
    setup_checker(qwq);
    acquire_spinlock(qwq, &lock);
    _for_in_list (node, list)
    {
    	printf("List Element: %p\n", node) ;
    }
    release_spinlock (qwq, &lock);
}
```

另外，和 C++ 中的 `for(auto x : list)` 语法一样，**在遍历过程中插入或删除节点可能导致混乱**，请务必小心。如删除当前循环变量指向的节点后，你需要手动将其更新，参考代码 `p=_detach_from_list(p)` 。

## **\_empty_list**

测试链表是否只有一个节点。

**Syntax** `#define _empty_list(list)`

**Parameters**

- list 要测试的链表指针。

**Return Value** bool 型，若链表只有一个节点，返回 true，否则返回 false。

**Remarks** 如需与其他操作配合使用，请**注意并发安全问题**。如

```c
if (!_empty_list(list)) do something
```

不能保证 do something 的时候 list 中仍然只有一个元素。

## **QueueNode**

QueueNode 是一个内嵌式的**无锁队列**（无需加锁即可保证并发安全）节 点，其内嵌式声明方式与 ListNode 类似，但需要作为队列头的 QueueNode\* 指针，且不能自行访问队列中的元素，只能使用给定的几个操作。

QueueNode 及其队列头的使用可参考下面代码

```c
QueueNode ∗head = NULL;
QueueNode x;
add_to_queue(&head , &x); // x is added to the queue
QueueNode ∗y = fetch_from_queue(&head); // y == &x, and head == NULL now
```

队列操作要求使用**指向队列头的指针**（即QueueNode \*\*），操作过程中会**更改队列头的值**。

## **add_to_queue**

向队列中加入节点。

**Syntax** `QueueNode* add_to_queue(QueueNode** head, QueueNode* node)`

**Parameters**

- head 指向队列头的指针。如前所述，队列头为QueueNode\*类型。

- node 要加入队列的节点。无需初始化节点。

**Return Value** Unused.

## **fetch_from_queue**

从队列中取出一个元素。

**Syntax** `QueueNode* fetch_from_queue(QueueNode** head)`

**Parameters**

- head 指向队列头的指针。如前所述，队列头为QueueNode\* 类型。

**Return Value** 指向取出的队列节点的指针。可以使用 container_of 得到对应的元素。

**Remarks** 虽然名字叫队列，但其实是按照**栈顺序（先进后出）**取出的。

## **fetch_all_from_queue**

取出队列中的所有元素。

**Syntax** `QueueNode* fetch_all_from_queue(QueueNode** head)`

**Parameters**

- head 指向队列头的指针。如前所述，队列头为QueueNode\*类型。

**Return Value** 指向一个队列节点的指针，可以将该节点作为单向链表头节点使用，遍历所有取出的元素。可参考下面示例代码

```c
void fetch_and_print_all (QueueNode∗∗ head )
{
    for(QueueNode∗ p = fetch_all_from_queue(head); p; p = p->next)
        printf("Queue Element : %p\n", p) ;
}
```

**注意：**上面代码绝对不能写成 f(QueueNode\* x) 然后在f中使用 &x，因为这样队列操作时更改的是f的形参，不是真正的队列头变量！

## **(Undocumented)**

-   `_merge_list`、`_insert_into_list`、`_detach_from_list` 同不带开头下划线的相应版本，但没有加锁和解锁的步骤，不保证并发安全。

# \<common/rc.h\>

## **RefCount**

RefCount是一个内嵌式的引用计数结构，可以使用RefCount.count访问计数的值，但请注意直接修改计数不是并发安全的，你应该使用提供的原子操作函数increment_rc和decrement_rc。

## **init_rc**

初始化引用计数结构体。

**Syntax** `init_rc(RefCount* rc)`

**Parameters**

-   rc 要初始化的 RefCount 结构体指针。

## **increment_rc**

执行引用，即引用计数加一。

**Syntax** `#define increment_rc(checker, rc)`

**Parameters**

-   checker Checker 名，用于检查引用与解引用是否配对。
-   rc 指向要增加引用计数的 RefCount 结构体的指针。

们要求所有引用操作与解引用操作形成配对。如有特殊需求，可以自行使用beginctx/endctx 增减 checker 计数，也可使用带下划线的无 checker 版本（不推荐）。

## **decrement_rc**

执行解引用，即引用计数减一。

**Syntax** `#define decrement_rc(checker, rc)`

**Parameters**

- checker Checker 名，用于检查引用与解引用是否配对。

- rc 指向要减少引用计数的 RefCount 结构体的指针。

**Remarks** 执行解引用操作会同时减少所给 checker 的计数。一般情况下， 我们要求所有引用操作与解引用操作形成配对。如有特殊需求，可以自行使用 beginctx/endctx 增减 checker 计数，也可使用带下划线的无 checker 版本（不推荐）。

## **(Undocumented)**

-   `_increment_rc`、`_decrement_rc` 同不带开头下划线的相应版本，但参数中不含 checker，不会进行配对计数和执行配对检查。

# \<common/spinlock.h\>

## **SpinLock**

SpinLock是一个自旋锁结构体，与本节中的一组函数配合使用。自旋 锁是一种最简单的锁，通过原子操作轮询锁的标志实现，轮询期间 CPU 会一直处于占用状态。对于极短时间操作，使用自旋锁效率很高，但也容易浪费 CPU 资源，不适合较长时间操作使用。

与所有其他锁一样，使用锁时，请务必注意**死锁问题**。

## **init_spinlock**

初始化自旋锁。

**Syntax** `void init_spinlock(SpinLock* lock)`

**Parameters**

-   lock 指向要初始化的自旋锁结构体的指针。

## **try_acquire_spinlock**

尝试取得自旋锁。如果不能立刻取得自旋锁，则返回失败。

**Syntax** `#define try_acquire_spinlock(checker, lock)`

**Parameters**

- checker Checker 名，用于检查取锁与放锁操作是否配对。

- lock 指向要尝试取得自旋锁的 SpinLock 结构体的指针。

**Return Value** bool 型，如果成功取得自旋锁，返回 true。如果不能立即取得自旋锁，放弃并返回 false。

**Remarks** 如果成功取得自旋锁，会增加 checker 计数。

## **acquire_spinlock**

轮询直到成功取得自旋锁。

**Syntax** `#define acquire_spinlock(checker, lock)`

**Parameters**

- checker Checker 名，用于检查取锁与放锁操作是否配对。

- lock 指向要取得自旋锁的 SpinLock 结构体的指针。

**Remarks** 此操作会增加 checker 计数。轮询可能浪费较多 CPU 时间，仅应在耗时极少的任务上使用此方式。

## **release_spinlock**

释放自旋锁。

**Syntax** `#define release_spinlock(checker, lock)`

**Parameters**

- checker Checker 名，用于检查取锁与放锁操作是否配对。

- lock 指向要释放自旋锁的 SpinLock 结构体的指针。

**Remarks** 此操作会减少 checker 计数。

## **(Undocumented)**

-   `_try_acquire_spinlock`、`_acquire_spinlock`、`_release_spinlock` 同不带开头下划线的相应版本，但参数中不含 checker，不会进行配对计数和执行配对检查。

# **\<common/string.h\>**

## **memset**

功能、参数、返回值与 C 标准库中的同名函数相同。

**Syntax** `void *memset(void *s, int c, usize n)`

## **memcpy**

功能、参数、返回值与 C 标准库中的同名函数相同。

**Syntax** `void *memcpy(void *restrict dest, const void *restrict src, usize n)`

**Remarks** 与 C 标准库中相同，memcpy不能处理 dest 与 src 重叠的情况，memmove可以。

## **memcmp**

功能、参数、返回值与 C 标准库中的同名函数相同。

**Syntax** `int memcmp(const void *s1, const void *s2, usize n)`

## **memmove**

功能、参数、返回值与 C 标准库中的同名函数相同。

**Syntax** `void *memmove(void *dest, const void *src, usize n)`

## **strncpy**

功能、参数、返回值与 C 标准库中的同名函数相同。

**Syntax** `char *strncpy(char *restrict dest, const char *restrict src, usize n)`

**Remarks** 与 C 标准库相同，此处的 n 将字符串末尾的\\0计入在内。如对于字符串"hello"，复制时需设置n\>=6，否则字符串末尾的\\0不会被复制， 复制是不完整的。

## **strncpy_fast**

功能、参数、返回值与strncpy基本一致，区别在于：若 src（含末尾\\0在内）的长度小于 n，strncpy会在 dest 末尾填 0 直至总计写入了 n 个字节的数据，而strncpy_fast不会继续填 0。

**Syntax** `char *strncpy_fast(char *restrict dest, const char *restrict src, usize n)`

## **strncmp**

功能、参数、返回值与 C 标准库中的同名函数相同。

**Syntax** `int strncmp(const char *s1, const char *s2, usize n)`

## **strlen**

功能、参数、返回值与 C 标准库中的同名函数相同。

**Remarks** 与 C 标准库相同，strlen 不计字符串末尾的\\0，如"hello"的 strlen=5。

**Syntax** `usize strlen(const char *s)`

# **<kernel/init.h\>**

## **Kernel Initializing**

我们将内核的初始化设计为下面流程：

1. 只启动 CPU0，其余 CPU 等待。

2. 初始化 BSS。

3. **执行注册的 early init 初始化函数**。UART 输出和物理内存分配器的初始化被注册为 early init 函数。

4. **执行注册的 init 初始化函数**。调度器和 root 进程的初始化被注册为 init 函数。

5. 启动所有 CPU，进入 idle 进程。

6. root 进程被调度，**执行注册的 rest init 初始化函数**。块设备和文件系统的初始化被注册为 rest init 函数。

7. root 进程转入用户态，执行/init。

三种初始化函数都是单线程执行的。以下三个宏提供了定义并注册初始化函数的功能，并可参考示例代码

```c
define_early_init(printk)
{
    init_spinlock(&printk_lock) ;
    return;//可以不写
}
```

## **define_early_init**

定义并注册 early init 阶段的初始化函数。

**Syntax** `define_early_init(name) { ... }`

**Parameters**

-   name 初始化函数的名字，可以与已有的变量和函数名重复，但不能与其他初始化函数名重复。
-   { ... } 初始化函数的函数体，可以像编写普通函数一样编写函数体， 也可以使用无返回值的 return 语句。

**Remarks** early init 阶段已经完成 bss 初始化，但尚不能使用printk和内存分配器。具体可参考本节开头的启动流程。**注册的各个 early init 函数的执行顺序是未定义的。**

## **define_init**

定义并注册 init 阶段的初始化函数。

**Syntax** `define_init(name) { ... }`

**Parameters**

- name 初始化函数的名字，可以与已有的变量和函数名重复，但不能与其他初始化函数名重复。

- { ... } 初始化函数的函数体，可以像编写普通函数一样编写函数体， 也可以使用无返回值的 return 语句。

**Remarks** init 阶段已经完成 early init 阶段初始化，且可以使用printk和内存分配器，但尚没有进程上下文。具体可参考本节开头的启动流程。**注册的各个 init 函数的执行顺序是未定义的。**

## **define_rest_init**

定义并注册 rest init 阶段的初始化函数。

**Syntax** `define_rest_init(name) { ... }` 

**Parameters**

- name 初始化函数的名字，可以与已有的变量和函数名重复，但不能与其他初始化函数名重复。

- { ... } 初始化函数的函数体，可以像编写普通函数一样编写函数体， 也可以使用无返回值的 return 语句。

**Remarks** rest init 阶段已经完成 init 阶段初始化，且已具有进程上下文， 可以执行与进程和调度相关的操作，但尚没有文件系统。具体可参考本节开头的启动流程。**注册的各个 rest init 函数的执行顺序是未定义的。**

## **(Undocumented)**

-   `do_early_init`、`do_init`、`do_rest_init` 分别执行所有 early init、init 和 rest init 初始化函数。前两者一般只应在 main 函数中由 cpu0 先后执行一次，后者一般只应在 kernel entry 中执行一次。