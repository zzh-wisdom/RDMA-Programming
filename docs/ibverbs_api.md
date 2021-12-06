# ibverbs

参考：<http://www.rdmamojo.com/>

- [1. 初始化](#1-初始化)
  - [1.1. ibv_fork_init](#11-ibv_fork_init)
- [2. RDMA设备](#2-rdma设备)
  - [2.1. ibv_get_device_list](#21-ibv_get_device_list)
  - [2.2. ibv_free_device_list](#22-ibv_free_device_list)
  - [2.3. ibv_get_device_name](#23-ibv_get_device_name)
  - [2.4. ibv_get_device_guid](#24-ibv_get_device_guid)
  - [2.5. 设备打开与关闭](#25-设备打开与关闭)
  - [2.6. 特定设备的查询函数](#26-特定设备的查询函数)
- [3. QP 创建](#3-qp-创建)
  - [3.1. 保护域 PD](#31-保护域-pd)
  - [3.2. 完成队列 cq](#32-完成队列-cq)
- [4. 内存区域 MR](#4-内存区域-mr)
- [5. 交换QP信息](#5-交换qp信息)
- [6. 发送和接受](#6-发送和接受)
- [7. CQ events](#7-cq-events)
- [8. 错误/意外事件](#8-错误意外事件)
- [9. Address Handles](#9-address-handles)

ibverbs API 总览 <https://www.rdmamojo.com/2012/05/18/libibverbs/>

## 1. 初始化

### 1.1. [ibv_fork_init](https://www.rdmamojo.com/2012/05/24/ibv_fork_init/)

前提：该函数应该在 libibverbs 中的其他任何函数调用前调用

ibv_fork_init()初始化 libibverbs 的数据结构以正确处理fork()函数调用并避免数据损坏，无论fork()是显式调用还是隐式调用（例如在system()、popen()等中）。

这个函数不常用。

## 2. RDMA设备

### 2.1. [ibv_get_device_list](https://www.rdmamojo.com/2012/05/31/ibv_get_device_list/)

```cpp
struct ibv_device** ibv_get_device_list(int *num_devices);
```

返回当前可用的**以 NULL 结尾**的 RDMA 设备数组。应该使用`ibv_free_device_list()`释放数组。

不应直接访问数组条目。相反，它们应该与以下服务动词一起使用： ibv_get_device_name()、ibv_get_device_guid()和ibv_open_device()。

**参数：**

num_devices: （可选）如果不为NULL，则设置为数组中返回的设备数。

**返回值：**

在成功时返回可用 RDMA 设备的数组，如果请求失败则返回 NULL 并设置errno。如果未找到设备，则将 num_devices 设置为 0，并返回非 NULL。

可能的errno值是：

- EPERM - 权限被拒绝。
- ENOMEM - 内存不足，无法完成操作。
- ENOSYS - 没有对 RDMA 的内核支持。

### 2.2. [ibv_free_device_list](https://www.rdmamojo.com/2012/06/07/ibv_free_device_list/)

ibv_free_device_list（） 释放 RDMA 设备数据列表。

释放数组后，指向未使用 ibv_open_device（） 打开的设备的指针将不再有效。客户端代码必须打开所有设备，它打算在调用ibv_free_device_list（）之前使用。

### 2.3. ibv_get_device_name

man ibv_get_device_name

```cpp
const char *ibv_get_device_name(struct ibv_device *device);
```

返回与 RDMA 设备设备关联的用户可读名称。

### 2.4. ibv_get_device_guid

```cpp
uint64_t ibv_get_device_guid(struct ibv_device *device);
```

返回 RDMA 设备device的全局唯一标识符 （GUID）

按网络字节顺序排列的设备 GUID。

### 2.5. 设备打开与关闭

```cpp
#include <infiniband/verbs.h>
struct ibv_context *ibv_open_device(struct ibv_device *device);
int ibv_close_device(struct ibv_context *context);
```

ibv_open_device() 打开设备 device 并创建上下文以供进一步使用。
失败时返回 NULL

ibv_close_device() 关闭上下文，成功返回0，失败则返回-1；

注意：

ibv_close_device（） 不会释放使用上下文context分配的所有资源。 为避免资源泄漏，**用户应在关闭上下文之前释放所有关联的资源**。

为防止资源（如内存、文件描述符、RDMA 对象编号）泄漏。使用这些孤立资源可能会导致分段错误。但是，当该过程结束时，操作系统将自动清理这些资源。

### 2.6. 特定设备的查询函数

```cpp
int ibv_query_device(struct ibv_context *context,
                            struct ibv_device_attr *device_attr);
```

ibv_query_device（） 返回具有上下文context的设备属性。 参数 device_attr 指向ibv_device_attr结构的指针，如 &lt;infiniband/verbs.h&gt; 中所定义。

成功时返回 0，失败时返回 errno 的值（指示失败原因）。

**注意：**

此函数返回的最大值是设备支持的资源的上限。 但是，可能无法使用这些最大值，因为可以创建的任何资源的实际数量可能会受到计算机配置、主机内存量、用户权限以及其他用户/进程已在使用的资源数量的限制。

注意：该函数已经过时，新函数如下：

```cpp
int ibv_query_device_ex(struct ibv_context *context,
                               struct ibv_device_attr_ex *attr);
```

返回具有上下文上下文的设备属性。 参数 attr 是指向ibv_device_attr_ex结构的指针，如 &lt;infiniband/verbs.h&gt; 中所定义。

成功时返回 0，失败时返回 errno 的值（指示失败原因）。

```cpp
int ibv_query_port(struct ibv_context *context, uint8_t port_num,
                          struct ibv_port_attr *port_attr);
```

ibv_query_port（） 通过指针port_attr返回设备上下文context的端口port_num 属性。 参数 port_attr 是一个ibv_port_attr结构，如 &lt;infiniband/verbs.h&gt; 中所定义。

成功时返回 0，失败时返回 errno 的值（指示失败原因）。

```cpp
int ibv_query_pkey(struct ibv_context *context,
                                 uint8_t port_num,
                                 int index,
                                 uint16_t *pkey);
```

ibv_query_pkey（） 通过指针 pkey 返回端口 port_num 的条目 index 中的P_Key值（按网络字节顺序）用于设备上下文context。

返回 RDMA 设备端口的 P_Key 表中的索引值。

成功时返回 0，出错时返回 -1。

```cpp
int ibv_query_gid(struct ibv_context *context,
                                uint8_t port_num,
                                int index,
                                union ibv_gid *gid);
```

查询 InfiniBand 端口的 GID 表

returns 0 on success, and -1 on error.

## 3. QP 创建

### 3.1. 保护域 PD

```cpp
struct ibv_pd *ibv_alloc_pd(struct ibv_context *context);
int ibv_dealloc_pd(struct ibv_pd *pd);
```

ibv_alloc_pd() 为 RDMA 设备上下文context分配 PD。失败时返回null。

ibv_dealloc_pd() 解除分配 PD pd。成功返回0，否则返回错误码。

### 3.2. 完成队列 cq

```cpp
struct ibv_cq *ibv_create_cq(struct ibv_context *context, int cqe,
                                    void *cq_context,
                                    struct ibv_comp_channel *channel,
                                    int comp_vector);

int ibv_destroy_cq(struct ibv_cq *cq);

int ibv_resize_cq(struct ibv_cq *cq, int cqe);
```

为 RDMA 设备上下文创建完成队列 (CQ)。

当发送或接收队列中未完成的工作请求完成时，工作完成将添加到该工作队列的 CQ。此工作完成表明未完成的工作请求已完成（不再被视为未完成）并提供有关它的详细信息（状态、方向、操作码等）。

可以**共享单个 CQ 以在多个 QP 之间发送、接收和共享。工作完成保存信息以指定 QP 编号和它来自的队列（发送或接收）**。

用户可以定义 CQ 的最小尺寸。实际创建的大小可以等于或大于此值。

参数：

- cqe: CQ的最小容量，可以为 [1..dev_cap.max_cqe]
- cq_context: 可选地，用户设置的、且被用于设置 struct ibv_cq 的 cq_context 的值。使用谓词 ibv_get_cq_event（） 等待完成事件通知时，将返回此值。
- channel: 可选地，将用于指示**新的工作完成**已添加到此 CQ 的完成事件通道。NULL 表示不会使用任何完成事件通道。
- comp_vector: 将用于对**完成事件**发出信号的 MSI-X 完成向量。如果这些中断的 IRQ 关联掩码已配置为将每个 MSI-X 中断分散到由不同内核处理，则**此参数**可用于将完成工作负载分散到多个内核上。值可以是 [0..context->num_comp_vectors）。

返回值：

成功则返回非NULL，否则设置相应的 errno：

- EINVAL	Invalid cqe, channel or comp_vector
- ENOMEM	Not enough resources to complete this operation

ibv_destroy_cq() 成功时返回 0，失败时返回 errno 的值（指示失败原因）。

如果**任何队列对仍与此 CQ 关联，则 ibv_destroy_cq（） 将失败**。

```cpp
struct ibv_qp *ibv_create_qp(struct ibv_pd *pd,
                                    struct ibv_qp_init_attr *qp_init_attr);

int ibv_destroy_qp(struct ibv_qp *qp);
```

<https://www.rdmamojo.com/2012/12/21/ibv_create_qp/>
创建的qp属性，由qp_init_attr指定。

**对于内敛的数据大小：**

对于这些设备，建议尝试创建具有所需消息大小的 QP，并在 QP 创建失败时继续减小它。

qp_init_attr 同时是传入和传出参数。

大多参数都需要通过不断设置和重试。

**使用反复试验，应该可以获得此特定 RDMA 设备的正确属性。**

## 4. 内存区域 MR

```cpp
struct ibv_mr *ibv_reg_mr(struct ibv_pd *pd, void *addr,
                                 size_t length, int access);

struct ibv_mr *ibv_reg_mr_iova(struct ibv_pd *pd, void *addr,
                              size_t length, uint64_t hca_va,
                              int access);

int ibv_dereg_mr(struct ibv_mr *mr);
```

ibv_reg_mr（） 注册与保护域 pd 关联的内存区域 （MR）。 MR 的起始地址是addr，其大小是length。 参数 access 描述了所需的内存保护属性;它是 0 或以下一个或多个标志的按位 OR：参考 man 文档。

ibv_reg_mr_iova() 与普通reg_mr相同，只是允许用户在通过 lkey 或 rkey 时指定 MR 的虚拟基址。内存区域中的偏移量计算为"addr + （iova - hca_va）"。为hca_va指定 0 与 IBV_ACCESS_ZERO_BASED 具有相同的效果。

ibv_dereg_mr（） 注销 MR mr。

**返回值**：

ibv_reg_mr（） / ibv_reg_mr_iova（） 返回指向已注册 MR 的指针，**如果请求失败，则返回 NULL**。 本地键 （L_Key） 字段 lkey 在发布带有 ibv_post_* 谓词的缓冲区时用作结构体 ibv_sge 的 lkey 字段，远程进程使用远程键 （R_Key） 字段 rkey 来执行 Atomic 和 RDMA 操作。 远程进程将此 rkey 作为传递给 ibv_post_send 函数的结构体ibv_send_wr rkey 字段。

ibv_dereg_mr（） 在成功时返回 0，或在失败时返回 errno 的值（指示失败原因）。

如果任何内存窗口仍绑定到此 MR，则 ibv_dereg_mr（） 将失败。

## 5. 交换QP信息

```cpp
int ibv_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
                  int attr_mask);
```

<https://www.rdmamojo.com/2013/01/12/ibv_modify_qp/>

修改队列对 （QP） 的属性

更改的属性描述了 QP 的发送和接收属性。在UC和RC QP中，这意味着将QP与远程QP连接。

在 Infiniband 中，应向子网管理员 (SA) 执行路径查询，以确定 QP 应配置哪些属性或作为最佳解决方案，请使用通信管理器 (CM) 或通用 RDMA CM 代理 (CMA)连接 QP。但是，有些应用程序更喜欢自己连接 QP，并通过**套接字**交换数据来决定要使用的 QP 属性。

**返回值：**

Value	Description
0	On success
errno	On failure and no change will be done to the QP
EINVAL	Invalid value provided in attr or in attr_mask
ENOMEM	Not enough resources to complete this operation

> 关于qp的状态：
> SQD：SQ_DRAINED。在 IBV_EVENT_SQ_DRAINED 事件之后，并确保 QP 处于 IBV_QPS_SQD 状态后，用户可以安全地开始修改发送队列属性，因为不再有任何正在进行的发送消息。因此，现在可以安全地修改 QP 的操作特性并将其转换回完全操作的 RTS 状态。

## 6. 发送和接受

```cpp
int ibv_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr,
                         struct ibv_recv_wr **bad_wr);
```

<https://www.rdmamojo.com/2013/02/02/ibv_post_recv/>

ibv_post_recv() 逐个检查**链表**中的所有条目，检查它是否有效，从中生成特定于硬件的接收请求，并将其添加到QP的接收队列的尾部，而无需执行任何上下文切换。RDMA 设备将在传入操作码后立即接收其中一个工作请求，该 QP 将使用接收请求 （RR）。如果由于接收队列已满或 WR 中的某个属性损坏而导致其中一个 WR 出现故障，它将立即停止并将指针返回到该 WR。

不与 SRQ 关联的 QP 将根据以下规则处理接收队列中的工作请求：

- 如果 QP 处于 RESET 状态，则应立即返回错误。但是，它们可能是一些不遵循此规则的低级驱动程序（以消除对数据路径的额外检查，从而提供更好的性能）并且在此状态下发布接收请求可能会被默默忽略。
- 如果 QP 处于 INIT 状态，则可以发布接收请求，但不会处理它们。
- 如果 QP 处于 RTR、RTS、SQD 或 SQE 状态，则可以发布接收请求并对其进行处理。
- 如果 QP 处于 ERROR 状态，则可以发布 Receive Requests 并且它们将在错误状态下完成。

如果 QP 与共享接收队列 (SRQ) 相关联，则必须调用ibv_post_srq_recv()而不是ibv_post_recv()，因为不会使用 QP 自己的接收队列。

**返回值:**

Value	Description
0	On success
errno	On failure and no change will be done to the QP and bad_wr points to the RR that failed to be posted
EINVAL	Invalid value provided in wr
ENOMEM	Receive Queue is full or not enough resources to complete this operation
EFAULT	Invalid value provided in qp

常见问题：

我可以知道工作队列中有多少 WR 未完成吗？
不，你不能。您应该根据**已发布的 WR 数量和您轮询的工作完成数量**来跟踪未完成的 WR 数量。

哪些操作会消耗RR？
如果远程端使用以下操作码之一发布send请求，将消耗一个 RR：

- Send
- Send with Immediate
- RDMA Write with immediate

ibv_post_recv() 返回后，我可以（重新）使用接收请求吗？
是的。该动词将接收请求从 libibverbs 抽象转换为特定于硬件的接收请求，您可以（重新）使用接收请求和其中的 s/g 列表。

```cpp
int ibv_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr,
                         struct ibv_send_wr **bad_wr);
```

<https://www.rdmamojo.com/2013/01/26/ibv_post_send/>

ibv_post_send（） 将工作请求 （WR） 的链接列表发布到队列对的发送队列 （QP）。ibv_post_send（）逐个检查链表中的所有条目，检查它是否有效，从中生成特定于硬件的发送请求，并将其添加到QP的发送队列的尾部，而无需执行任何上下文切换。RDMA 设备将（稍后）以异步方式处理它。**如果其中一个 WR 由于发送队列已满或 WR 中的某个属性不正确而导致故障，它将立即停止并返回指向该 WR 的指针**。QP 将根据以下规则处理"发送"队列中的工作请求：

- 如果 QP 处于 RESET、INIT 或 RTR 状态，则应立即返回错误。但是，它们可能是一些不遵循此规则的低级驱动程序（以消除数据路径中的额外检查，从而提供更好的性能），并且在一种或所有这些状态下发布发送请求可能会被静默忽略。
- 如果 QP 处于 RTS 状态，则可以发布发送请求并对其进行处理。
- 如果 QP 处于 SQE 或 ERROR 状态，则可以发布发送请求，这些请求将**在错误的情况下完成**。
- 如果 QP 处于 SQD 状态，则可以发布发送请求，**但不会处理这些请求**。

```cpp
int ibv_poll_cq(struct ibv_cq *cq, int num_entries,
                       struct ibv_wc *wc);
```

<https://www.rdmamojo.com/2013/02/15/ibv_poll_cq/>

"Work Completion"表示工作队列中的工作请求以及所有与 CQ 关联的工作队列中未完成的未发送信号的工作请求**已完成**。任何"接收请求"、"发送请求"和以错误结尾的"发送请求"将在处理结束后生成"工作完成"。

```cpp
const char *ibv_wc_status_str(enum ibv_wc_status status);
```

返回一个字符串，该字符串描述"工作完成状态"枚举值。

## 7. CQ events

```cpp
       struct ibv_comp_channel *ibv_create_comp_channel(struct ibv_context
                                                        *context);

       int ibv_destroy_comp_channel(struct ibv_comp_channel *channel);
```

<https://www.rdmamojo.com/2012/10/19/ibv_create_comp_channel/>

ibv_create_comp_channel（） 为 RDMA 设备上下文创建完成事件通道。

此完成事件通道是由 libibverbs 引入的抽象，在 InfiniBand 架构谓词规范或 RDMA 协议谓词规范中不存在。完成事件通道实质上是文件描述符，用于将工作完成通知传递到用户空间进程。为完成队列 （CQ） 生成工作完成事件时，该事件将通过（附加到该 CQ 的）完成事件通道传递。这对于通过使用多个完成事件通道将完成事件引导到不同的线程或为不同的 CQ 提供不同的优先级可能很有用。

**一个或多个完成队列可以与同一个完成事件通道相关联**。

```cpp
int ibv_req_notify_cq(struct ibv_cq *cq, int solicited_only);
```

ibv_req_notify_cq（） 请求完成队列 （CQ） 上的完成通知

ibv_req_notify_cq（） 请求在将**特定请求类型的下一个工作完成**添加到 CQ 时发出通知。在调用 ibv_req_notify_cq（） 之前 CQ 中存在的任何工作完成都不会导致创建完成通知。完成通知将使用ibv_get_cq_event（）读取。

可以请求两种类型的完成事件：

- 请求的Solicited完成事件 - 当带有请求（Solicited）事件指示器集set的传入send或 RDMA 写入带立即数消息（即，远程端发布了一个发送请求，并在send_flags中设置了IBV_SEND_SOLICITED）导致生成一个**成功的接收工作完成**或任何不成功的（发送的接收）工作完成添加到 CQ 时发生。
- 未经请求的Unsolicited完成事件 - 当任何工作完成被添加到 CQ 时发生，无论它是发送还是接收工作完成，以及它是成功还是不成功完成工作。

如果请求完成通知是挂起的，即调用了ibv_req_notify_cq（）并且没有发生完成通知，则对具有相同 CQ 请求相同完成事件类型的ibv_req_notify_cq（）的后续调用将不起作用;**只会生成一个完成通知**。调用 ibv_req_notify_cq（） 仅在发生完成通知后才生效。

对于同一 CQ，对下一个完成事件的 Request Completion Notification 优先于 solicited 的 Request Completion Notification。也就是前面所说的，后者优先于前者。

一旦完成通知发生，如果一个人希望获得更多的完成通知，他必须再次调用ibv_req_notify_cq（）。

参数：0 则是非 solicited，非0则是 solicited。

使用事件处理工作完成将减少应用程序的 CPU 消耗;您的进程将休眠，直到新的工作完成将添加到 CQ 中。

请求有关 Solicited 完成项的通知仅适用于与**接收队列关联的 CQ**。

```cpp
       int ibv_get_cq_event(struct ibv_comp_channel *channel,
                            struct ibv_cq **cq, void **cq_context);

       void ibv_ack_cq_events(struct ibv_cq *cq, unsigned int nevents);
```

ibv_get_cq_event（） 等待下一个完成事件, 根据使用 ibv_req_notify_cq（） 为特定完成事件通道请求的完成事件类型。

默认情况下，ibv_get_cq_event（） 是一个阻塞函数，如果没有任何要读取的完成事件，它将等到生成下一个完成事件。使用专用线程等待下一个完成事件发生会很有用。但是，如果希望以非阻塞方式读取事件，则可以执行此操作。可以使用 fcntl（） 将完成事件通道通道中事件文件的文件描述符配置为非阻塞，然后使用 read（）/poll（）/epoll（）/select（） 读取此文件描述符，以确定是否存在任何等待读取的完成事件。在这篇文章中有一个关于如何做到这一点的例子。

使用 ibv_get_cq_event（） 接收的所有完成事件都必须使用 **ibv_ack_cq_events（）** 进行确认。

使用完成事件的典型用法如下：

第一阶段：准备

1. 创建与完成事件通道关联的 CQ
2. 在新的（第一个）完成事件时发出通知的请求

第二阶段：完成处理例程

3. 等待完成事件并确认它
4. 请求在下一个完成事件时发出通知
5. Empty the CQ

请注意，在 CQ 中没有相应的"工作完成"条目的情况下，可能会触发额外的事件。如果在步骤 4 和步骤 5 之间将完成条目添加到 CQ，然后在步骤 5 中清空（轮询）CQ，则会发生这种情况。就是触发了事件，但cq却没有对应的工作完成条目。

## 8. 错误/意外事件

```cpp
int ibv_get_async_event(struct ibv_context *context,
                               struct ibv_async_event *event);

void ibv_ack_async_event(struct ibv_async_event *event);
```

ibv_get_async_event（） 读取 RDMA 设备上下文context的下一个异步事件。

调用 ibv_open_device（） 后，所有**异步事件都将排队到此上下文中**，并且调用 ibv_get_async_event（） 将按顺序逐个读取它们。即使ibv_get_async_event（） 将在事件生成后很长一段时间内被调用，它仍然会首先读取较旧的事件。不幸的是，事件没有任何时间概念，用户无法知道事件发生的时间。

默认情况下，ibv_get_async_event（） 是一个阻塞函数，如果没有任何要读取的异步事件，它将一直等到生成下一个事件。使用专用线程等待下一个事件发生会很有用。但是，如果希望以非阻塞方式读取事件，则可以执行此操作。可以使用 fcntl（） 将设备上下文中事件文件的文件描述符配置为非阻塞，然后使用 read（）/poll（）/epoll（）/select（） 读取此文件描述符，以确定是否存在等待读取的事件。在这篇文章中有一个关于如何做到这一点的例子。

调用 ibv_get_async_event（） 是原子的，即使在多个线程中调用它，也可以保证同一事件不会被多个线程读取。

使用 ibv_get_async_event（） 接收的每个事件都必须使用 ibv_ack_async_event（） 进行确认。

<https://www.rdmamojo.com/2012/08/11/ibv_get_async_event/> 这里有异步读事件的例子。

## 9. Address Handles

```cpp
struct ibv_ah *ibv_create_ah(struct ibv_pd *pd,
                                    struct ibv_ah_attr *attr);

int ibv_destroy_ah(struct ibv_ah *ah);
```

创建与保护域关联的地址句柄 （AH）。

稍后，当发送请求 （SR） 发布到不可靠的数据报 QP 时，将使用此 AH。
