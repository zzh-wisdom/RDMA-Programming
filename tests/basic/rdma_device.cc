#include <stdio.h>

#include <endian.h>

#include <infiniband/verbs.h>

int main()
{
    struct ibv_device **dev_list;
    int num_devices, i;

    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list)
    {
        perror("Failed to get IB devices list");
        return 1;
    }

    printf("    %-16s\t   node GUID\n", "device");
    printf("    %-16s\t----------------\n", "------");

    for (i = 0; i < num_devices; ++i)
    {
        printf("    %-16s\t%016llx\n",
               ibv_get_device_name(dev_list[i]),
               (unsigned long long)be64toh(ibv_get_device_guid(dev_list[i])));
    }
    // for (i = 0; i < num_devices; ++i)
    // {
    //     printf("    %-16s\t%016llx\n",
    //            ibv_get_device_name(dev_list[i]),
    //            ibv_get_device_guid(dev_list[i]));
    // }
    if(num_devices == 0) return 0;
    printf("\n");

    ibv_context* ibv_ctx = ibv_open_device(dev_list[0]);
    if(ibv_ctx == nullptr) {
        fprintf(stderr, "ibv_open_device fail.");
        return -1;
    }

    ibv_device_attr device_attr;
    int err = ibv_query_device(ibv_ctx, &device_attr);
    if(err) {
        fprintf(stderr, "ibv_query_device error: %d\n", err);
        return -1;
    }
    printf("phys_port_cnt: %u\n", device_attr.phys_port_cnt);
    printf("max_cq: %u\n", device_attr.max_cq);
    printf("max_mr: %u\n", device_attr.max_mr);
    printf("max_mr_size: %lu\n", device_attr.max_mr_size);
    printf("max_qp: %u\n", device_attr.max_qp);
    printf("max_qp_wr: %u\n", device_attr.max_qp_wr);
    printf("max_sge: %u\n", device_attr.max_sge);
    printf("max_sge_rd: %u\n", device_attr.max_sge_rd);

    ibv_free_device_list(dev_list);

    return 0;
}
