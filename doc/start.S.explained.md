# More About Lab1：理解 `start.S`

> **注意**
>
> 这是对 `start.S` 的详细介绍，感兴趣的同学可以阅读了解。**但不要求掌握，因为这不是 Lab1 的一部分，和评分也没有关系，也不要求在报告中体现。**

本文将按照 `start.S` 的结构，逐步介绍 `start.S` 的内容。

## 1. 预定义头

```c
/* SCR_EL3, Secure Configuration Register (EL3). */
#define SCR_RESERVED (3 << 4)

// 直到…

#define MAIR_VALUE             ((MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | \
                                (MT_NORMAL_FLAGS << (8 * MT_NORMAL)) | \
                                (MT_NORMAL_NC_FLAGS << (8 * MT_NORMAL_NC)))
```

定义了后面所需的一些寄存器值的常量。

> **你知道吗**
>
> - `.S` 文件是需要经过预编译器处理的汇编文件，通常特指使用 GAS 或 Clang 汇编编译器编译的源代码。它可以包含 `#include`、`#define` 等预处理指令，以及汇编指令，但必须先经过预处理器处理才能编译。
> - `.s` 文件是已经经过预编译器处理的汇编文件，通常特指使用 GAS 或 Clang 汇编编译器编译的源代码。它只可包含汇编指令。
> - `.asm` 文件是汇编文件，通常特指使用 NASM/YASM/FASM/MASM/JWASM 等汇编编译器编译的源代码。它只可包含汇编指令。

```c
#define KERNEL_STACK_SIZE 4096
```

定义了每个 CPU 可使用的内核栈的大小（以字节计），这里是 4KB。

> **注意**
>
> 如果发现有奇怪的越界访问和结构体损坏时，请优先考虑是否有栈空间溢出的可能。通常来说不会有这种问题，但是如果你的代码中使用了大量的栈空间（例如递归），那么就需要注意了。


## 2. 代码段

```c
.section ".text.boot"
```

表示后面的代码在 `.text.boot` 段中。

### 2.1. `_start`

```assembly
mov x15, x0
```

因为后面会用到通用寄存器 `x0`，所以先将其原内容保存到另一个通用寄存器 `x15` 中。

```assembly
adr     x0, entry
mov     x1, #1
mov     x2, #2
mov     x3, #3
```

`entry` 是一个标签，表示 `entry` 的地址。`adr`（**ad**d**r**ess） 指令会将 `entry` 的地址载入 `x0`。

```assembly
mov     x9, 0xd8
str     x0, [x9, x1, lsl #3]
str     x0, [x9, x2, lsl #3]
str     x0, [x9, x3, lsl #3]
```

`str`（**st**o**r**e） 指令会将 `entry` 的地址存储到 `[x9 + 1 * 8]`、`[x9 + 2 * 8]`、`[x9 + 3 * 8]` 处。其中 `lsl #3` 表示左移（**l**eft **s**hift） 3 位，即乘以 8。

这里写入 `0xd8` 用于唤醒其他 CPU，并令他们直接跳转到 `entry` 处执行。


```assembly
dsb     sy
isb

sev
```

`dsb`（**d**ata **s**ynchronization **b**arrier）和 `isb`（**i**nstruction **s**ynchronization **b**arrier）围栏指令分别用于同步（即排空）数据和指令流水线。这么做通常是为了保证数据和指令流在各个处理器间同步。

`sy`（full **sy**nc）是数据同步等级的选项，可参考[文档](https://developer.arm.com/documentation/ddi0596/2021-12/Base-Instructions/DSB--Data-Synchronization-Barrier-?lang=en)。

`sev`（**s**end **ev**ent）指令用于向其他 CPU 发送一次事件，这里用于实在地唤醒其他 CPU。

### 2.2. `entry`

```assembly
/* get current exception level from CurrentEL[3:2]. */
mrs     x9, CurrentEL
and     x9, x9, #0xc
cmp     x9, #8
beq     el2
bgt     el3
```

获取当前的异常等级。如果 ``x9`` 的值为 8（即 ``2 << 2``），则表示当前处于 EL2，跳转到 `el2` 处执行。如果 ``x9`` 的值大于 8（即 ``3 << 2``），则表示当前处于 EL3，跳转到 `el3` 处执行。

> **你知道吗**
>
> 异常等级（**E**xception **L**evel）是 ARMv8 架构中的概念，用于区分不同的特权级别。ARMv8 架构中共有 4 个异常等级，由低到高分别是 EL0、EL1、EL2 和 EL3。EL0 是用户态，EL1 是内核态，EL2 和 EL3 是虚拟化相关的特权级别，分别用于运行「虚拟化管理器」Hypervisor 和「虚拟机安全监控器」Secure Monitor，本实验只会用到 EL0 和 EL1。
>
> 每个等级都会有自己的一批专用寄存器集合，例如 `sp`（**S**tack **P**ointer），`spsr`（**S**aved **P**rogram **S**tatus **R**egister）等。之所以叫「异常等级」，是因为在低等级模式下发生异常时，通常会立刻切换到高一级的模式下执行异常处理程序。例如在 EL0 下发生异常时，会立刻切换到 EL1 下执行异常处理程序。

### 2.3. `el3`

```assembly
mov     x9, #SCR_VALUE
msr     scr_el3, x9
mov     x9, #SPSR_EL3_VALUE
msr     spsr_el3, x9
adr     x9, el2
msr     elr_el3, x9
eret
```

这段代码用于在 EL3 下初始化，并从 EL3 跳回到 EL2。

`scr_el3`（**S**ecure **C**onfiguration **R**egister）是系统控制寄存器，用于控制 Secure Monitor 的行为。这里我们将其值设置为 `SCR_VALUE`，表示（所有寄存器的位的含义可从[文档](https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/SCR-EL3--Secure-Configuration-Register?lang=en)中查看，下不赘述）：

1. `SCR_RW`：表示所有异常等级都是 AArch64，而非 AArch32；
2. `SCR_HCE`：表示启用 `hvc`（**h**ypervisor **v**irtual **c**all）指令，用于从 EL1~3 跳转到 EL2；
3. `SCR_SMD`：表示禁用 `smc`（**s**ecure **m**onitor **c**all）指令，它用于从 EL1~3 跳转到 EL3；
4. `SCR_NS`：表示所有低于 EL3 的异常等级都是非安全的，不允许其访问 EL3 定义的安全内存区域。

`spsr_el3`（**S**aved **P**rogram **S**tatus **R**egister）是（上一异常等级的）程序状态寄存器，用于储存 EL2 的状态，会在回到 EL2 时恢复到 `cpsr_el2`（**C**urrent **P**rogram **S**tatus **R**egister）中。

`elr_el3`（**E**xception **L**ink **R**egister）是异常链接寄存器，用于储存跳回时回到的地址。

注意这些寄存器都属于专用寄存器，只能在 EL3 下访问，不能直接使用 `mov` 指令进行赋值，需要使用 `msr`（**m**ove to **s**ystem **r**egister）指令。

这里我们给 `elr` 的值是 `el2` 程序的首地址，这样当从 EL3 跳回到 EL2 时，就会跳转到 `el2` 处执行。

`eret`（**e**xception **ret**urn）指令用于从异常处理程序返回，会恢复到上一异常等级，并将 `elr` 中的地址载入 `pc`（**P**rogram **C**ounter）寄存器，从而跳转到 `el2` 处执行。

### 2.4. `el2`

和 `el3` 程序基本一致，不再赘述。

执行后会回到 EL1，并跳转到 `el1` 继续执行。

### 2.5. `el1`

```c
adr     x9, kernel_pt

/* higher and lower half map to same physical memory region. */
msr     ttbr0_el1, x9
msr     ttbr1_el1, x9
```

`kernel_pt` 是在 `src/aarch64/kernel_pt.c` 中定义的一个页表，用于将物理内存映射到虚拟内存。

关于页表的内容会在后面的实验和课程中详细说明，此处不解释。

```assembly
ldr     x9, =TCR_VALUE
msr     tcr_el1, x9

ldr     x9, =MAIR_VALUE
msr     mair_el1, x9
```

`tcr_el1`（**T**ranslation **C**ontral **R**egister）是转换控制寄存器，用于控制虚拟地址到物理地址的转换行为。

`mair_el1`（**M**emory **A**ttribute **I**ndirection **R**egister）是内存属性间接寄存器，用于控制虚拟地址的内存属性。

这两个寄存器的值也可参考文档。

```assembly
/* enable MMU. */
mrs     x9, sctlr_el1
orr     x9, x9, #SCTLR_MMU_ENABLED
msr     sctlr_el1, x9

isb
```

`mrs`（**m**ove from **s**ystem **r**egister）指令用于从系统寄存器中读取值，和 `msr` 指令正好相反。

`orr`（**or** **r**egister）指令用于将两个寄存器的值进行或运算，并将结果写入第一个寄存器。

`sctlr_el1`（**S**ystem **C**on**t**ro**l** **R**egister）是系统控制寄存器，用于控制系统的行为，这里我们设置它来启动 MMU（**M**emory **M**anagement **U**nit，内存管理单元）。

```assembly
/* read CPUID. */
mrs     x0, mpidr_el1
and     x0, x0, #3

/* sp = _start - CPUID * KERNEL_STACK_SIZE */
mov     x10, #KERNEL_STACK_SIZE
mul     x10, x10, x0
ldr     x9, =_start
sub     x9, x9, x10

/* set up stack pointer, must always be aligned to 16 bytes. */
msr     spsel, #1
mov     sp, x9
```

读取 `mpidr_el1`（**M**ulti**p**rocessor **Id**entification **R**egister）寄存器的值，它包含标识 CPU 的 ID，在树莓派上可以为 0~3。

然后，我们利用 `mpidr_el1` 的值计算出当前 CPU 的内核栈的栈顶地址，即 `_start - id * KERNEL_STACK_SIZE`。这里的 `_start` 是我们首行指令对应的地址，前面有定义。

最后，我们把 `spsel`（**S**tack **P**ointer **Sel**ector）寄存器改为 1，表示使用 `sp_el1` 寄存器作为当前的栈指针 `sp`，然后将栈指针设置为上一步计算出的值。

### 2.6. 最终跳转

```
mov x0, x15
ldr     x9, =main
br      x9
```

这里我们把 `x0` 的值还原回去（如果不记得这行的作用，回去看文件的第一行！），然后载入我们的 `main` 的地址，跳转到 `main` 函数执行。

> **你知道吗**
>
> 我们用到了 `adr` 和 `ldr` 两个命令，它们都是用于载入地址的，那么它们有什么区别呢？
>
> - `adr x0, some_label` 是一个**伪指令**，会被编译器编译成 `add` 或者 `sub`。假如这段代码实际在 `0x30000000` 运行，那么 `adr x0, some_label` 得到形如 `x0 = 0x30000123` 的结果；而如果这段代码实际在 `0x40000000` 运行，那么就是 `x0 = 0x40000123` 了；
> - `ldr x0, some_label` 是一个**指令**，会被编译器编译成 `ldr`。**它的实际效果通常和 `adr` 完全一样**。和汇编程序计算相对于 `pc` 的偏移量，并生成相对于 `pc` 的索引指令：`ldr x0, [pc, #123]`；
> - `ldr x0, =some_label` 是一个**伪指令**，会被编译器编译成 `ldr`。汇编程序会要求链接器负责确定地址，生成绝对地址指令。例如，如果链接器被配置为 `. = 0x40000000`，那么无论程序实际在哪里执行，`ldr x0, =some_label` 都会被编译成类似于 `ldr x0, #0x40000123` 的指令。
>
> **太长不看**：`adr` 和 `ldr` 通常是一样的，用实际运行地址 `pc` 来索引，`ldr = ` 则是获得尊重链接器配置的地址。

> **思考**（这不是思考题！）
>
> 为什么在设置虚拟内存之后，我们就只能用 `ldr = `，但在设置之前，只能用 `adr`？