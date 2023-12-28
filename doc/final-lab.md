## File Descriptor
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


## Shell
### 解释说明
欢迎来到最后一部分！本部分将决定你的操作系统是否能够真正地运行起来（~~以及你的分数。如果你能顺利完成本部分，你已经超过了大约一半的同学~~）。

### 实现内容
(1) 本部分中你将补全一些用户态程序的实现，它们包括：

- [cat(1)](https://linux.die.net/man/1/cat)：`src/user/cat/main.c`
- [mkdir(1)](https://linux.die.net/man/1/mkdir)：`src/user/mkdir/main.c`

(2) 另外，不要忘了让你的内核启动后自动执行第一行用户态程序 `src/user/init.S`！你需要在 `src/kernel/core.c` 的 `kernel_entry()` 中手动创建并启动第一个用户态进程。