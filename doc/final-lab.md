# lab10 Final Lab

本次实验大家将完成最后的整合，实现各种 system call 与最后的 shell。

需要完成的内容中，特殊标记的含义

（T）工具函数，如果使用别的实现可以不写这个

（O）不实现相关功能也可以启动shell，如果时间来不及，请优先保证你能启动shell

## Console

> 本模块内的相关问题推荐联系戴江凡助教。

存在一片内核缓冲区和记录已经被读取的(r)，已经写入的(w)，光标所在正在写的(e)。

```c
struct {
    char buf[INPUT_BUF];
    usize r;  // Read index
    usize w;  // Write index
    usize e;  // Edit index
} input;
```

数组是环形数组。数组满时不能继续写入。遇到`\n`和`ctrl+D`时将未写入部分变为写入，即更新w到e处。

<u>**TODO**</u> 你需要完成`kernel/console.c`中的下列函数。

### console_intr

处理串口中断。使用`uart_put_char`和`uart_get_char`来完成相关操作。

编辑（写入，`backspace`）缓冲区内容，并回显到console。

使用`uart_get_char`获取串口读入字符，完成下列情况：

1. `backspace`：`\x7f`，删除前一个字符，`input.e--`。但是位于w前的已写入字符不能删除。（思考如何回显出删除的效果）。会用到`uart_put_char('\b')`，效果是光标向左移动一位。
2. `Ctrl+U`：删除一行。
3. `Ctrl+D`：更新`input.w`到`input.e`。表示`EOF`。本身也要写入缓冲区和回显。
4. 普通字符写入和回显。

回显会用到`uart_put_char`向console写入内容。

可以适当自定义`Ctrl+<字母>`，如果有特殊定义，请在报告中说明。

### console_write

```
isize console_write(Inode *ip, char *buf, isize n)
```

将`buf`中的内容在console显示。

需要锁`inode ip`。返回n。

### console_read

```
isize console_read(Inode *ip, char *dst, isize n)
```

读出console缓冲区的内容n个字符到`dst`。遇见`EOF`提前结束。返回读出的字符数。



## Pipe

> 本模块内的相关问题推荐联系戴江凡助教。

```c
typedef struct pipe {
    SpinLock lock;
    Semaphore wlock,rlock;
    char data[PIPESIZE];
    u32 nread;      // number of bytes read
    u32 nwrite;     // number of bytes written
    int readopen;   // read fd is still open
    int writeopen;  // write fd is still open
} Pipe;
```

**<u>TODO</u>** 请完成`fs/pipe.c`中的下列函数。

### pipeAlloc（O）

```
int pipeAlloc(File** f0, File** f1)
```

创建`pipe`并初始化，创建`pipe`两端的`File`放入`f0`,`f1`,分别对应读和写。

成功返回0，失败返回-1.

### pipeClose（O）

```
void pipeClose(Pipe* pi, int writable)
```

关闭`pipe`的一端。如果检测到两端都关闭，则释放`pipe`空间。

### pipeWrite（O）

```
int pipeWrite(Pipe* pi, u64 addr, int n)
```

向`pipe`写入n个byte，如果缓冲区满了则sleep。返回写入的byte数。

### pipeRead（O）

```
int pipeRead(Pipe* pi, u64 addr, int n)
```

从`pipe`中读n个byte放入addr中，如果`pipe`为空并且writeopen不为0，则sleep，否则读完pipe，返回读的byte数。

### sysfile.c：pipe2（O）

分配的`pipe`，并将`pipe`的`f0`,`f1`放入第一个参数指向的地址。

## File Descriptor

> 本模块内的相关问题推荐联系许冬助教

### 解释说明
欢迎来到文件系统的最后一部分！在这一部分，我们将完成和文件（File）以及文件描述符有关的最终实现。

你也许有疑问：为什么已经有了完整的硬盘文件/目录抽象（Inode），还需要再包装一层？答案正如我们所常说：Unix 的设计哲学是**一切皆文件**，而硬盘文件以外，我们还有**设备文件**（Device File）和**管道文件**（Pipe File）等等。这些文件的特点是：它们不是硬盘上的文件，而是内核中的一些数据结构，但和硬盘文件一样都可以作为读写操作的对象。

因此，我们需要为它们再设计一套抽象，使得用户态程序可以通过同一套机制来访问它们中的任何一个。这套机制就是**文件**（File）。

文件除了需要持有底层对象以外，还有一些额外的字段：

- 文件以偏移量（对于硬盘文件就是从文件开头的字节数，对于设备文件和管道文件就是迄今为止读/写的字节数）来标识当前读写的位置；
- 文件为了能同时被多个进程持有，需要记录当前持有者的数量；
- 文件还需要记录当前可行的读写模式（例如，设备文件可能仅能以只读模式打开，管道文件可能一头只写、一头只读，没有权限写的硬盘文件可能也仅能以只读模式打开）。

为了节省不必要的开支，所有的文件对象被组织在**一个全局文件表**中（其实就是一个普通的数组），具体到每个进程则又有**自己的进程文件表**（同样是一个数组）。进程文件表中，每个文件对象都有一个**文件描述符**（File Descriptor）来标识自己。文件描述符是一个非负整数，即进程文件表中的下标，通过它可以找到对应的文件对象。

一个极简的文件对象如下所示：

```c
enum file_type {
    NONE,    // 未使用
    DEVICE,  // 设备文件
    INODE,   // 硬盘文件
    PIPE,    // 管道文件
};

struct file {
    enum file_type type;    // 文件类型
    bool readable, writable;// 读写模式
    int refcnt;             // 当前持有者数量
    usize offset;           // 当前读写偏移量
    union {
        struct inode *inode;    // 硬盘文件的下层接口（Inode）
        struct device *device;  // 设备文件的下层接口（Device）
        struct pipe *pipe;      // 管道文件的下层接口（Pipe）
    };
};
```

不过值得注意的是，在本实验中我们进行了一些简化和修改：

1. 我们的外设太少了（只有一个终端字符设备），而且我们目前还没有做出虚拟文件系统（VFS）的接口，**因此为简便起见，我们不再为设备设计一个单独的抽象，而是直接将硬盘 Inode 兼看作设备文件的下层接口。** 也就是说，当我们创建一个设备文件时，我们将在硬盘上创建一个实在的 Inode，其类型为 `INODE_DEVICE`。因此，**你的 Inode 接口也要考虑到设备文件的情况**（例如 `inode_read` 等函数可能调用设备文件的下层接口，如 `console_read`）。

### 实现内容
你需要实现的内容如下：

(1) `src/fs/file.c`：

```c
// === 文件的创建/关闭/元信息读取操作 ===
// 从全局文件表中分配一个空闲的文件
struct file* file_alloc();
// 获取文件的元信息（类型、偏移量等）
int file_stat(struct file* f, struct stat* st);
// 关闭文件
void file_close(struct file* f);

// === 读写 file 对应文件的内容 ===
// 将长度为 n 的 addr 写入 f 的当前偏移处
isize file_read(struct file* f, char* addr, isize n);
// 将长度为 n 的 addr 从 f 的当前偏移处读入
isize file_write(struct file* f, char* addr, isize n);

// === 辅助操作 ===
// 文件的引用数+1
struct file* file_dup(struct file* f);
// 初始化全局文件表
void init_ftable();
// 初始化/释放进程文件表
void init_oftable(struct oftable*);
void free_oftable(struct oftable*);
```

(2) 另外，截止目前我们都未编写路径字符串（例如 `/this/is//my/path/so_cool.txt`）的解析逻辑。我们在 `src/fs/inode.c` 中实现了比较困难的部分，你需要补完剩余的 Todo 函数。

(3) 最后，为了对接用户态程序，我们需要在 `src/kernel/sysfile.c` 中实现一些系统调用：

- [close(3)](https://linux.die.net/man/3/close)
- [chdir(3)](https://linux.die.net/man/3/chdir)

(4) 以及几个辅助函数：

```c
// 从描述符获得文件
static struct file *fd2file(int fd);
// 从进程文件表中分配一个空闲的位置给 f
int fdalloc(struct file *f);
// 根据路径创建一个 Inode
Inode *create(const char *path, short type, short major, short minor, OpContext *ctx);
```

(5) 别忘了上面「解释说明」中提到的值得注意的地方。


## fork() 和 execve()

> 本模块内的相关问题推荐联系赵行健助教

本模块可以说是操作系统最核心的部分，我们的操作系统通过这两个函数加载和启动用户程序。进行本部分任务前，建议先完成File Descriptor。

**<u>TODO</u>**

* `kernel/syscall.c: user_readable user_writeable`
  
  检查syscall中由用户程序传入的指针所指向的内存空间是否有效且可被用户程序读写
- `kernel/proc.c: fork TODO`

- `kernel/exec.c: execve TODO`

- `kernel/pt.c: copyout TODO`（T）

- `kernel/paging.c: copy_sections TODO`（T）

- `kernel/sysfile.c: mmap TODO`（O）

- `kernel/sysfile.c: munmap TODO`（O）

- `kernel/paging.c init_sections`: 不需要再单独初始化 heap 段了

- `kernel/paging.c pgfault`: 增加有关文件的处理逻辑（O）

#### ELF 可执行文件格式

ELF 文件描述了一个程序。它可以看作一个程序的“镜像”，就像ISO文件是一个光盘的“镜像”一样。ELF 文件包含程序的入口地址、各个段的大小、各个段的属性（可读、可写、可执行等）等等。我们按照这些信息，将程序加载到内存中，然后跳转到入口地址，就可以运行这个程序了。

在本部分中，我们需要实现 ELF 文件的解析和加载。ELF 格式的具体定义可以参见 [https://en.wikipedia.org/wiki/Executable_and_Linkable_Format](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) 。在这里，我们给出两个会用到的结构体定义：

相关头文件：`musl/include/elf.h`

```C
typedef struct {
unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    Elf64_Word    p_type;
    Elf64_Word    p_flags;
    Elf64_Off     p_offset;
    Elf64_Addr    p_vaddr;
    Elf64_Addr    p_paddr;
    Elf64_Xword   p_filesz;
    Elf64_Xword   p_memsz;
    Elf64_Xword   p_align;
} Elf64_Phdr;
```

注意：ELF 文件中的 section header 和lab的 struct section并不一样，本次实验不用考虑 section header 的情况

#### fork() 系统调用

创建一个当前进程的完整复制，通过`fork()`的返回值区分父进程和子进程

从进程的结构体入手，依次判断其中的变量是否需要复制，是否需要修改

为了配合 fork()，你可能需要在原先的 `UserContext` 中加入所有寄存器的值。此外，你还需要保存`tpidr0`和`q0`，因为musl libc会使用它们。

#### execve() 系统调用

替换当前进程为filename所指的 ELF 格式文件，并开始运行该文件（变身）

> 进程的哪些部分需要释放，哪些部分不需要？

```C
int execve(const char* filename, char* const argv[], char* const envp[]) 
//filename: 标识运行的可执行文件
//argv: 运行文件的参数（和 main 函数中的 argv 指的是一个东西）
//envp: 环境变量

//  execve 异常返回到可执行文件的入口，即elf.e_entry（可以认为就是以下格式的main函数），而在main函数看来依然是一般的函数调用
int main(int argc, char *argv[]) 
//入口函数，其中argc表示参数的数量，argv表示参数的指针数组，比如ls ..  其中 argc=2， argv[0]:"ls", argv[1]:".."

//example code
char * argv[ ]={"ls","..",(char *)0};
char * envp[ ]={"PATH=/bin",(char*)0};
if(fork()==0)
    execve("ls", argv, envp);
else

//user stack structure
/*
 * Step1: Load data from the file stored in `path`.
 * The first `sizeof(struct Elf64_Ehdr)` bytes is the ELF header part.
 * You should check the ELF magic number and get the `e_phoff` and `e_phnum` which is the starting byte of program header.
 *
 * Step2: Load program headers and the program itself
 * Program headers are stored like: struct Elf64_Phdr phdr[e_phnum];
 * e_phoff is the offset of the headers in file, namely, the address of phdr[0].
 * For each program header, if the type(p_type) is LOAD, you should load them:
 * A naive way is 
 * (1) allocate memory, va region [vaddr, vaddr+filesz)
 * (2) copy [offset, offset + filesz) of file to va [vaddr, vaddr+filesz) of memory
 * Since we have applied dynamic virtual memory management, you can try to only set the file and offset (lazy allocation)
 * (hints: there are two loadable program headers in most exectuable file at this lab, the first header indicates the text section(flag=RX) and the second one is the data+bss section(flag=RW). You can verify that by check the header flags. The second header has [p_vaddr, p_vaddr+p_filesz) the data section and [p_vaddr+p_filesz, p_vaddr+p_memsz) the bss section which is required to set to 0, you may have to put data and bss in a single struct section. COW by using the zero page is encouraged)

 * Step3: Allocate and initialize user stack.
 * The va of the user stack is not required to be any fixed value. It can be randomized. (hints: you can directly allocate user stack at one time, or apply lazy allocation)
 * Push argument strings.
 * The initial stack may like
 *   +-------------+
 *   | envp[m] = 0 |
 *   +-------------+
 *   |    ....     |
 *   +-------------+
 *   |   envp[0]   |  ignore the envp if you do not want to implement
 *   +-------------+
 *   | argv[n] = 0 |  n == argc
 *   +-------------+
 *   |    ....     |
 *   +-------------+
 *   |   argv[0]   |
 *   +-------------+
 *   |    argc     |
 *   +-------------+  <== sp

 * ## Example
 * sp -= 8; *(size_t *)sp = argc; (hints: sp can be directly written if current pgdir is the new one)
 * thisproc()->tf->sp = sp; (hints: Stack pointer must be aligned to 16B!)
 * The entry point addresses is stored in elf_header.entry
*/ 
```

在`execve()`中，一般可执行文件中可以加载的只有两段：RX的部分+RW的部分（其它部分会跳过）（因此只设置两种状态，一种是RX，另一种是RW）

- RX的部分： 代码部分，主要包括 text 段，属性为 ST_FILE + RO
- RW的部分：数据部分，包括了 data + bss 段

#### copyout（T）

> 复制内容到给定的页表上，在exec中，为了将一个用户地址空间的内容复制到另一个用户地址空间上，可能需要调用这样的函数。如果你使用其他实现，则不需要实现这个函数。

```C
/*                                        
 * Copy len bytes from p to user address va in page table pgdir.
 * Allocate a file descriptor for the given file.
 * Allocate physical pages if required.
 * Takes over file reference from caller on success.
 * Useful when pgdir is not the current page table.
 * return 0 if success else -1
 */                                                          
int copyout(struct pgdir* pd, void* va, void *p, usize len)
```

## File Mapping (O)

> 本模块内的相关问题推荐联系赵行健助教

### 什么是 `mmap()` ？
mmap, 从函数名就可以看出来这是memory map, 即内存映射, 是一种内存映射文件的方法, 将一个文件或者其它对象映射到进程的地址空间，实现文件磁盘地址和进程虚拟地址空间中一段虚拟地址的一一对映关系。mmap()系统调用使得进程之间通过映射同一个普通文件实现共享内存。普通文件被映射到进程地址空间后，进程可以向访问普通内存一样对文件进行访问，不必再调用read()，write()等操作。

注：实际上，mmap()系统调用并不是完全为了用于共享内存而设计的。它本身提供了不同于一般对普通文件的访问方式，进程可以像读写内存一样对普通文件的操作。而Posix或系统V的共享内存IPC则纯粹用于共享目的，当然mmap()实现共享内存也是其主要应用之一。

### 为什么要用 `mmap()` ？
Linux通过内存映像机制来提供用户程序对内存直接访问的能力。内存映像的意思是把内核中特定部分的内存空间映射到用户级程序的内存空间去。也就是说，**用户空间和内核空间共享一块相同的内存**。这样做的直观效果显而易见：内核在这块地址内存储变更的任何数据，用户可以立即发现和使用，根本无须数据拷贝。举个例子理解一下，使用mmap方式获取磁盘上的文件信息，只需要将磁盘上的数据拷贝至那块共享内存中去，用户进程可以直接获取到信息，而相对于传统的write/read　IO系统调用, 必须先把数据从磁盘拷贝至到**内核缓冲区中(页缓冲)**，然后再把数据拷贝至用户进程中。两者相比，**mmap会少一次拷贝数据**，这样带来的性能提升是巨大的。

使用内存访问来取代read()和write()系统调用能够简化一些应用程序的逻辑。
在一些情况下，它能够比使用传统的I/O系统调用执行文件I/O这种做法提供更好的性能。
原因是：

1. 正常的read()或write()需要两次传输：一次是在文件和内核高速缓冲区之间，另一次是在高速缓冲区和用户空间缓冲区之间。使用mmap()就不需要第二次传输了。对于输入来讲，一旦内核将相应的文件块映射进内存之后，用户进程就能够使用这些数据了；对于输出来讲，用户进程仅仅需要修改内核中的内容，然后可以依靠内核内存管理器来自动更新底层的文件。
2. 除了节省内核空间和用户空间之间的一次传输之外，mmap()还能够通过减少所需使用的内存来提升性能。当使用read()或write()时，数据将被保存在两个缓冲区中：一个位于用户空间，另个一位于内核空间。当使用mmap()时，内核空间和用户空间会共享同一个缓冲区。此外，如果多个进程正在同一个文件上执行I/O，那么它们通过使用mmap()就能够共享同一个内核缓冲区，从而又能够节省内存的消耗。

本部分为可选内容，我们提供了用户程序`mmaptest`以测试你的实现。

#### mmap munmap（O）

相关定义`musl/include/sys/mman.h`
 
参考资料：[XV6学习（15）Lab mmap: Mmap - 星見遥 - 博客园](https://www.cnblogs.com/weijunji/p/xv6-study-15.html)

```C
void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
```

可以利用我们之前实现的 section 数据结构


## Shell

> 本模块内的相关问题推荐联系许冬助教

### 解释说明
欢迎来到最后一部分！本部分将决定你的操作系统是否能够真正地运行起来（~~以及你的分数。如果你能顺利完成本部分，你已经超过了大约一半的同学~~）。

### 实现内容
(1) 本部分中你将补全一些用户态程序的实现，它们包括：

- [cat(1)](https://linux.die.net/man/1/cat)：`src/user/cat/main.c`
- [mkdir(1)](https://linux.die.net/man/1/mkdir)：`src/user/mkdir/main.c`

(2) 另外，不要忘了让你的内核启动后自动执行第一行用户态程序 `src/user/init.S`！你需要在 `src/kernel/core.c` 的 `kernel_entry()` 中手动创建并启动第一个用户态进程。

## 测试与提交

编译用户态程序需要先使用`make libc -j`编译musl libc，编译一次后后面即可直接使用。

当你完成全部任务后，如果一切顺利，将进入一个shell，你可以自己运行里面的程序。我们也编写了一个usertests程序可供运行（你可以自己改一改来加强测试）。

请注意助教最终不一定会只使用公布的测试程序来测试你的代码。

**<u>Final Lab的提交时间为1月26日，如有特殊情况请提前联系助教，一般不接受迟交。</u>**