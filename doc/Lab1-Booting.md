# Lab 1: Booting

本学期，我们将实现一个简单的操作系统内核。第一个实验里，我们的目标是引导系统进入 `main` 函数，输出 `Hello world!` 。

## 实验环境

本学期实验使用 AArch64 架构，推荐在服务器上进行，已为大家提供独立的容器和工具。

连接服务器的命令：

```shell
ssh root@10.176.34.211 -p 你的端口号
```

需要本地拥有 ssh 的私钥，Windows 和 Linux 下都可运行，推荐使用 VS Code 的 Remote 插件进行连接。

容器有两个特殊的目录（挂载点）。`/share` 目录只读，供所有同学共享，我们会在其中放置一些共享文件。`~/data` 目录可读写，我们为你挂载了一块4.4T的硬盘，建议你在该目录下存储实验所用到的文件。

CPU、内存和硬盘资源是所有同学共享的，我们没有为大家设置使用配额，请注意合理使用资源。输入 `htop` 查看硬件资源的使用情况，q 键退出

在服务器上开发，可以参考下面指令初始化代码仓库，你也可以 fork 一份你自己的仓库

```bash
cd ~/data
# clone代码仓库
git clone https://github.com/idlebo/OS-23Fall-FDU.git
cd OS-23Fall-FDU
# 切换到本实验分支
git checkout lab1
# 新建一个dev分支
git checkout -b lab1-dev
# 创建build目录用来进行后续的运行
mkdir -p build
cd build
cmake ..
make qemu 
```

之后每次运行只要在 `build` 目录下 `cmake .. && make qemu` 。

**`qemu` 的退出方法为： `Ctrl+A`，松开后按 `x`。**

每次发布新的 lab 后，可以参考下面指令更新代码仓库

```bash
# 拉取远端仓库
git fetch --all
# 提交你的更改
git add .
git commit -m "your commit message"
# 切换到新lab的分支
git checkout lab2
# 新建一个分支，用于开发
git checkout -b lab2-dev
# 引入你在上个lab的更改
git merge lab1-dev
```

如果合并发生冲突，请参考错误信息自行解决。

## AArch64

> AArch64 等价于 ARMv8 的 64 位指令集。

本学期的教学 OS 将运行在 AArch64 架构下。

绝大多数代码都使用 C 语言完成，部分代码需要使用汇编语言编写，建议大家掌握 AArch64 汇编的下面内容

指令手册：[Arm Architecture Reference Manual for A-profile architecture](https://developer.arm.com/documentation/ddi0487/latest/)

* 各通用寄存器及其别名、函数调用约定、栈结构

* 算术、访存、跳转等基本汇编指令

## Booting

> 操作系统之获得控制权的两个要素：内核模式与优先引导

#### 内核模式

AArch64 中有 EL3、EL2、EL1、EL0 四个特权级，我们的 lab 只使用 EL1、EL0 特权级。其中 EL1 就是运行操作系统的内核模式，EL0 是用户模式，硬件保障了不同模式下程序运行的权限不同，从而让用户程序始终在操作系统的管理下。

#### 优先引导

在插上电源开机之后，就需要运行操作系统的内核，然后由操作系统来管理计算机。

问题在于，操作系统作为一个程序，必须加载到内存里才能执行。而“把操作系统加载到内存里”这件事情，不是操作系统自己能做到的，就好像你不能拽着头发把自己拽离地面。

因此我们可以想象，在操作系统执行之前，必然有一个其他程序执行，他作为“先锋队”，完成“把操作系统加载到内存“这个工作，然后他功成身退，把 CPU 的控制权交给操作系统。

这个“其他程序”，我们一般称之为 bootloader。很好理解：他负责 boot (开机)，还负责 load (加载OS到内存里)，所以叫 bootloader。

QEMU 模拟的树莓派自带 bootloader，这里有关于 bootloader 的[简要介绍](https://blog.csdn.net/zxnsirius/article/details/52166558)。

操作系统的二进制可执行文件被 bootloader 加载到内存中（在树莓派中加载到物理地址 0x80000 处，这个用 gdb 看不到，已经做好了），然后 bootloader 会把CPU的"当前指令指针"(pc, program counter)跳转到内存里的一个位置，开始执行内存中那个位置的指令（在树莓派中这个 pc 也是 0x80000，这个用 gdb 可以看到的，感兴趣的同学可以自行使用 gdb 调试），这样操作系统就占据了先机。

> 因为可以优先运行，操作系统在开机配置完成后确保自己掌握了整个系统的控制权

## .init section

在内核初始化时，需要调用很多模块的初始化函数。借助链接器脚本，我们可以将需要在初始化时调用的函数指针都放置到 `.init` section中，内核初始化时遍历调用 `.init` section中放置的所有函数指针，即可完成所有模块的初始化。

> 这个设计不适用于需要精确控制初始化顺序的情况。

## 链接器脚本

我们使用了链接器脚本控制内核入口（ `_start` 函数）刚好被链接在生成的内核文件的开头（并完成其他的内存布局），所用的脚本可参见`linker.ld`。链接器脚本参考：[Linker Scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)。用到的一些语法：

* `.` 被称为 location counter，代表了当前位置代码的实际上会运行在内存中的哪个地址。如 `. = 0xFFFF000000080000;` 代表我们告诉链接器内核的第一条指令运行在 0xFFFF000000080000。但事实上，我们一开始是运行在物理地址 0x80000（为什么仍能正常运行？），开启页表后，我们才会跳到高地址（虚拟地址），大部分代码会在那里完成。

* text 段中的第一行为 `KEEP(*(.text.boot))` 是指把 `start.S` 中我们自定义的 `.text.boot` 段放在 text 段的开头。`KEEP` 则是告诉链接器始终保留此段，否则编译器可能会将未用到的段优化掉。

* `PROVIDE(etext = .)` 声明了一个符号 `etext`（但并不会为其分配具体的内存空间，即在 C 代码中我们不能对其进行赋值），并将其地址设为当前的 location counter。这使得我们可以在汇编或 C 代码中得到内核各段（text/data/bss）的地址范围，我们在初始化 BSS 的时候需要用到这些符号。

我们不要求大家自行编写链接器脚本。

`_start` 主要完成一些架构相关的初始化工作：唤醒所有 CPU 核（如何唤醒所有 CPU 核），在EL3、EL2特权级下进行简单的配置，进入EL1特权级，使用 `kernel_pt` 开启虚拟地址（开启虚拟地址的瞬间发生了什么？），设置内核栈，最后跳转到 `main` 函数。

`kernel_pt` 是 `aarch64/kernel_pt.c` 中预定义的页表，它基本上就是直接映射，映射关系为虚拟地址等于物理地址加上 `0xffff0000_00000000`。`kernel_pt` 还标记了一些用于MMIO的地址为 `PTE_DEVICE` ，提示cache需要对该地址特殊处理。关于页表的细节我们会在后续lab中介绍。

## putchar

与之前在计组实验中做的类似，我们使用MMIO方式访问UART设备（字符设备），向固件设定的地址写一字节数据，即可输出一个字符。

为避免该操作被编译器优化影响，我们需要为地址加上 `volatile` 标记，以免编译器将其优化掉，并添加编译器屏障，以免编译器将其乱序处理，可参考 `src/aarch64/intrinsic.h` : `device_put_u32()` 。

## <u>作业与提交</u>

> `aarch64/intrinsic.h` 提供了一些架构相关的代码。
>
> `kernel/init.h` 提供了一些初始化相关的代码。
>
> `driver/uart.h` 提供了一些uart相关的代码。
>
> `common/string.h` 提供了一些字符串相关的代码。

在 `main.c` 的 `main` 函数中添加代码，实现下面功能：

* 仅CPU0进入初始化流程，其余CPU直接退出。（我们没有实现电源管理功能，CPU退出可以参考提供的 `arch_stop_cpu()` 函数）

* 初始化 `.bss` 段，将其填充为 `0` 。（为什么？[这里有简要说明](https://www.zhihu.com/question/23147702)）

* 先后调用 `do_early_init()` 和 `do_init()` 函数。

* OS的 `main` 函数不能返回，请在完成前面步骤后让CPU0退出。

在 `main.c` 中使用 `define_early_init` 定义函数1，该函数将 `hello[]` 填充为 `Hello world!` 。

在 `main.c` 中使用 `define_init` 定义函数2，该函数通过 uart 输出 `hello[]` 的内容。

> 我们后续会直接为大家提供用法类似于 `printf` 的 `printk` 函数，无需大家自行对uart进行进一步封装。

如果一切顺利，你将看到`Hello world!`（怎么实现的？）

**提交：将实验报告提交到 elearning 上，格式为`学号-lab1.pdf`。本次实验中，<u>报告不计分</u>。<u>截止时间：9月15日 12:00</u>。逾期提交将扣除部分分数。**

报告中可以包括下面内容

* 代码运行效果展示

* 实现思路和创新点

* 思考题

* 对后续实验的建议

* 其他任何你想写的内容