# ibverbs

参考：<http://www.rdmamojo.com/>

## 1. 初始化

## 2. [ibv_fork_init](https://www.rdmamojo.com/2012/05/24/ibv_fork_init/)

前提：该函数应该在 libibverbs 中的其他任何函数调用前调用

ibv_fork_init()初始化 libibverbs 的数据结构以正确处理fork()函数调用并避免数据损坏，无论fork()是显式调用还是隐式调用（例如在system()、popen()等中）。

这个函数不常用。

## 3. RDMA设备

### 3.1. [ibv_get_device_list](https://www.rdmamojo.com/2012/05/31/ibv_get_device_list/)

```cpp
struct ibv_device** ibv_get_device_list(int *num_devices);
```

返回当前可用的**以 NULL 结尾**的 RDMA 设备数组。应该使用`ibv_free_device_list()`释放数组。

不应直接访问数组条目。相反，它们应该与以下服务动词一起使用：ibv_get_device_name()、ibv_get_device_guid()和ibv_open_device()。

**参数：**

num_devices: （可选）如果不为NULL，则设置为数组中返回的设备数。

**返回值：**

在成功时返回可用 RDMA 设备的数组，如果请求失败则返回 NULL 并设置errno。如果未找到设备，则将 num_devices 设置为 0，并返回非 NULL。

可能的errno值是：

- EPERM - 权限被拒绝。
- ENOMEM - 内存不足，无法完成操作。
- ENOSYS - 没有对 RDMA 的内核支持。

### ibv_free_device_list

