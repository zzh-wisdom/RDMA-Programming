# TODO

1. ODP: <https://community.mellanox.com/s/article/understanding-on-demand-paging--odp-x>
2. ibv_alloc_dm 设备内存怎么使用, 实验室的网卡不支持
3. 测试参数max_qp_rd_atom的影响，即作为目标，同时RDMA READ的个数
4. 好好测试各个参数对性能的影响

国外写的有关rdma编程的论文 <https://thegeekinthecorner.wordpress.com/2013/02/02/rdma-tutorial-pdfs/>
有一篇关于流控的可以参考

## 可能影响性能的参数

QP的MTU
max_qp_rd_atom（设置qp的属性，查询device的属性）
