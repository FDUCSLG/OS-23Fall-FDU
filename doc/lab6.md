# OS Lab 6 - Intro to Log FS & Block Device Cache

本次实验的目的是熟悉日志文件系统和块设备缓存的实现。

> *操作系统的本质：虚拟化、并发、**持久化***

> **一些有帮助的资源**
>
> - 聊聊 xv6 中的文件系统：https://www.cnblogs.com/KatyuMarisaBlog/p/14366115.html
> - xv6 中文文档：https://th0ar.gitbooks.io/xv6-chinese/content/content/chapter6.html

## 0. 更新

> **来源**
>
> 本部分来自 [2022 年的实验](https://github.com/FDUCSLG/OS-2022Fall-Fudan/blob/lab6/doc/lab6.md)。为方便大家实现，今年加入了一样的机制。**需要大家阅读后对之前的代码进行微调。**


为便于实现 Lab 6，添加了一套 Unalertable Waiting 机制，可供大家使用。

* 为 `wait_sem` 等一系列函数添加了编译检查，未处理返回值将编译失败。
  
  > **在 `wait_sem` 返回 `false` 后，应尽快返回用户态以处理 signal（killed），严禁再次 wait。** （为什么？）在 Lab 3、4、5 中，我们均未对是否正确处理 wait 被打断的情况作出要求，**从 Lab 6 开始，这一点将被纳入打分标准**。

* 添加了一套以 `unalertable_wait_sem` 为代表的函数，unalertable wait 不会被 signal（kill）打断，也没有返回值，**但请严格限制使用，保证 unalertable wait 能够在可预见的短时间内被唤醒，确保 kill 操作的时效性。** 关于 unalertable wait 的更多说明，请参考代码。

* 为实现 unalertable wait，参考 Linux 的 uninterruptable wait，我们为 `procstate` 添加了一项 `DEEPSLEEPING`，`DEEPSLEEPING` 与 `SLEEPING` 基本相同，区别仅在于 alert 操作对 `DEEPSLEEPING` 无效。**你需要在调度器相关代码中将 `DEEPSLEEPING` 纳入考虑。** 除 activate 操作外，一般可以将 `DEEPSLEEPING` 视为 `SLEEPING` 处理。

* 将原先的 `activate_proc` 重命名为 `_activate_proc`，添加了参数 `onalert`，以指出是正常唤醒还是被动打断，并用宏定义 `activate_proc` 为 `onalert = false` 的 `_activate_proc`，`alert_proc` 为 `onalert=true` 的`_activate_proc`。**你需要修改 `_activate_proc` 的代码，并在 `kill` 中换用 `alert_proc` 唤醒进程。**

## 1. 文件系统扫盲
### 1.1. 从磁盘到块设备抽象

在计算机中，我们基本把除了 CPU 和内存以外的所有东西当作外设，与这些外设进行交互的过程称为输入/输出（I/O）。

Linux 系统中，我们将所有外设抽象成**设备**（Device），设备分为三种：

1. 字符设备（Character Device）：以字符（二进制流）为单位进行 I/O 的设备，比如键盘、鼠标、串口等；
2. 块设备（Block Device）：以块（Block）为单位进行 I/O 的设备，比如硬盘；
3. 网络设备（Network Device）：以数据包（Packet）为单位进行 I/O 的设备，比如网卡。

在 Linux 中，设备进一步被抽象成了特殊的设备文件（File），因此我们可以通过文件系统的接口来访问设备。设备文件的路径常常为`/dev/<device_name>`，比如 `/dev/sda` 就是第一个 SATA 硬盘。

块设备的本质是**实现了以下两个接口的任何对象**：

```c
// 块大小
#define BLOCK_SIZE 512
// 从设备读取数据
int block_read(int block_no, uint8_t buf[BLOCK_SIZE]);
// 将数据写入设备
int block_write(int block_no, const uint8_t buf[BLOCK_SIZE]);
```

### 1.2. 从缓存到块缓存

计算机学科中处理重复的、低速的任务的办法是什么？**缓存**。它是最简单的空间换时间办法，在不同领域还有很多其他名字：动态规划、记忆化搜索……

相对于主存，磁盘是慢的。因此，理所当然地，我们需要在内存中维护一个**块缓存**（Block Cache），用于加速对块设备的访问。

块缓存自然是写回的，因此我们需要一个**脏位**（Dirty Bit）来标记块缓存中的数据是否已经被修改过。

不过，由于我们无法追踪块缓存中的数据是否被修改过，因此我们需要让用户在使用块缓存的时候**手动**标记脏位。这个过程称为**同步**（Sync）；另一方面，由于对同一块的写是互斥的，应该有个类似锁的机制来实现「不让同一块落入两个人手里」。

因此，块缓存的接口如下：

```c
// 从设备读取一块
Block *block_acquire(int block_no);
// 标记一块为脏
void block_sync(Block *block);
// 释放一块
void block_release(Block *block);
```

从另一个角度来看，也可以把前两者看成一组：它们的作用其实和块设备的两个函数类似：一个负责读，一个负责写。最后一个函数则是释放块缓存。

### 1.3. 从内存布局到磁盘布局

内存和磁盘都是存储设备，然而由于其本身的特性，两者的布局方式有很大的不同。

我们的文件系统布局如下（以 0 为文件系统起点）：

> **注意**
>
> 这里的 0 代表我们的文件系统的起始块号，不是实际的 SD 卡的起始块号。SD 卡布局参考 Lab 5 文档中相关布局。

| 起始块号 | 长度 | 用途               |
| -------- | -------- | ------------------ |
| 0        | 1        | 超级块             |
| `log_start`        | `num_log_blocks` | 日志区域           |
| `inode_start` | `num_inodes * sizeof(InodeEntry) / BLOCK_SIZE` | inode 区域         |
| `bitmap_start` | `num_bitmap_blocks` | 位图区域        |
| `data_start` | `num_data_blocks` | 数据区域           |

内存擅长小块随机读写，而硬盘只能大块整读整写，因此位图的效率反而比其他内存中使用过的动态分配器（例如 SLAB）要高。

硬盘和内存的分配器的接口是类似的：

```c
// 内存
void *kalloc(size_t size);
void kfree(void *ptr);

// 硬盘
int block_alloc();
void block_free(int block_no);
```

### 1.4. 日志文件系统是干什么的？

位于磁盘上的文件系统需要面临的一个问题是：当系统 crash 或者意外掉电的时候，如何维持**数据的一致性**。

比如现在为了完成某项功能，需要同时更新文件系统中的两个数据结构 A 和 B，因为磁盘每次只能响应一次读写请求，势必造成 A 和 B 其中一者的更新先被磁盘接收处理。如果正好在其中一个更新完成后发生了掉电或者 crash，那么就会造成不一致的状态。

因此，日志作为一种**写入协议**，基本思想是这样的：**在真正更新磁盘上的数据之前，先往磁盘上写入一些信息，这些信息主要是描述接下来要更新什么**。

一个典型的写入过程如下：

1. 将要写入的数据先写入磁盘的日志区域
2. 将日志区域的数据写入磁盘的数据区域
3. 将日志区域的数据删除

一个典型的系统启动中的初始化过程如下：

1. 检查日志区域是否有数据
2. 如果有，说明上次系统非正常终止，将日志区域的数据写入磁盘的数据区域
3. 将日志区域的数据删除

这个过程中任意时刻发生 crash 都是安全的，顶多是丢数据，但不会导致一致性问题。（**为什么？**）

日志协议的思想核心是：**持久化脏块本身**。

### 1.5. 事务：文件系统上的操作抽象

在日志文件系统中，我们将**一次文件系统操作**称为一个**事务**（Transaction）。一个事务中可能包含多个写块的操作，但是这些操作要么全部成功，要么全部失败。

先看接口：

```c
// 开始事务
void begin_op(OpContext *ctx);
// 结束事务
void end_op(OpContext *ctx);
```

这两个函数的作用请参考代码注释。我们的设计中已经有很多这样的「对称」方法（**你能举出其他例子吗？**），这种设计下我们往往更关注的是其中区域（Scope）的含义。

**事务的 Scope 中进行的是对块缓存的操作。** 当所有人都离开 Scope 后，事务才会真正生效（即，按照日志协议读写块缓存）。

> **注意**
>
> 本实验为简单起见，按照日志协议时块缓存总表现为写直达，不使用写回，以确保数据真正落盘。

> **思考**
>
> 我们的日志区域大小固定且有限，假如有 10000 个进程，每个进程都写了各不相同的 100 个块，然后一起执行 `end_op`，怎么办？
>
> 分批写入行不行？如果行，怎么分批？如果不行，为什么？

由于我们的事务不需要存储每个具体操作的内容（**为什么？**），因此我们只需要一个计数器来记录当前事务中的操作数即可。（不过这带来的缺点是我们也不能像数据库一样，回滚一个事务。）

## 2. 评测

有趣的是，文件系统本身作为一个抽象实现，理论上只要提供了正确的接口（例如块设备、内存分配器、锁等），本身应当是具有良好跨平台性的。

本次实验我们使用了基于 Mock 的评测方法，离开树莓派环境来测试你的文件系统。相关 C/C++ 代码在 `src/fs/test` 目录下。

我们仅 mock 了以下方法，因此请保证你只调用了在前面实验中出现的这些方法（理论上你也不该用到其他方法，否则说明你的实现不具有跨平台性）：

```c
void *kalloc(isize x);
void kfree(void *ptr);
void printk(const char *fmt, ...);
void yield();

// list.c 宏/方法集
// SpinLock 方法集
// Semaphore 方法集
// PANIC、assert 宏
// RefCount 方法集
```

首次评测时请在 `src/fs/test` 下执行：

```sh
$ mkdir build
$ cd build
$ cmake ..
```

之后每次评测时请在 `src/fs/test/build` 下执行：

```sh
$ make && ./cache_test
```

如果报错，请尝试清理：

```sh
$ make clean
```

通过标准：最后一行出现 `(info) OK: 23 tests passed.`。此外助教会通过测试的输出判断测试是否在正常运行。

以下是（上一届）助教的实现的输出：

```plaintext
(info) "init" passed.
(info) "read_write" passed.
(info) "loop_read" passed.
(info) "reuse" passed.
(debug) #cached = 20, #read = 154
(info) "lru" passed.
(info) "atomic_op" passed.
(fatal) assertion failed: "i < OP_MAX_NUM_BLOCKS"
(info) "overflow" passed.
(info) "resident" passed.
(info) "local_absorption" passed.
(info) "global_absorption" passed.
(info) "replay" passed.
(fatal) cache_alloc: no free block
(info) "alloc" passed.
(info) "alloc_free" passed.
(info) "concurrent_acquire" passed.
(info) "concurrent_sync" passed.
(info) "concurrent_alloc" passed.
(info) "simple_crash" passed.
(trace) running: 1000/1000 (844 replayed)
(info) "single" passed.
(trace) running: 1000/1000 (224 replayed)
(info) "parallel_1" passed.
(trace) running: 1000/1000 (168 replayed)
(info) "parallel_2" passed.
(trace) running: 500/500 (221 replayed)
(info) "parallel_3" passed.
(trace) running: 500/500 (229 replayed)
(info) "parallel_4" passed.
(trace) throughput = 20751.62 txn/s
(trace) throughput = 21601.20 txn/s
(trace) throughput = 18314.84 txn/s
(trace) throughput = 21231.88 txn/s
(trace) throughput = 18097.45 txn/s
(trace) throughput = 21225.89 txn/s
(trace) throughput = 18049.48 txn/s
(trace) throughput = 21035.98 txn/s
(trace) throughput = 17995.50 txn/s
(trace) throughput = 21220.89 txn/s
(trace) throughput = 17972.01 txn/s
(trace) throughput = 20875.56 txn/s
(trace) throughput = 18040.48 txn/s
(trace) throughput = 21422.29 txn/s
(trace) throughput = 18128.94 txn/s
(trace) throughput = 21169.92 txn/s
(trace) throughput = 18037.98 txn/s
(trace) throughput = 21279.36 txn/s
(trace) throughput = 18271.36 txn/s
(trace) throughput = 21325.34 txn/s
(trace) throughput = 17884.06 txn/s
(trace) throughput = 21182.41 txn/s
(trace) throughput = 18100.45 txn/s
(trace) throughput = 21194.90 txn/s
(trace) throughput = 18414.29 txn/s
(trace) throughput = 20993.50 txn/s
(trace) throughput = 18106.95 txn/s
(trace) throughput = 21266.87 txn/s
(trace) throughput = 17812.59 txn/s
(trace) throughput = 21317.34 txn/s
(trace) running: 30/30 (7 replayed)
(info) "banker" passed.
(info) OK: 23 tests passed.
```



## 3. 提交

**提交：将实验报告提交到 eLearning 上，格式为`学号-lab6.pdf`。**

**从 Lab 2 开始，用于评分的代码以实验报告提交时为准。如果需要使用新的代码版本，请重新提交实验报告。**

**<u>截止时间：2023 年 11 月 16 日 19:30</u>。逾期提交将扣除部分分数。**

报告中可以包括下面内容

- 代码运行效果展示（测试通过截图）

- 实现思路和创新点

- 对后续实验的建议

- 其他任何你想写的内容

- ~~不放可爱猫猫要扣分的！（不是）~~

报告中不应有大段代码的复制。如有使用本地环境进行实验的同学，请联系助教提交代码（最好可以给个 GIT 仓库）。使用服务器进行实验的同学，助教会在服务器上检查，不需要另外提交代码。
