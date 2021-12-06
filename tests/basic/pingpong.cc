#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <malloc.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

const uintptr_t test_cq_context = 0; // 0x12345678

// 对应：
// ibv_rc_pingpong --size 4096 --iters 1000000
// ibv_rc_pingpong --size 4096 --iters 1000000 localhost

unsigned int cfg_iters = 1000000;
int cfg_buf_size = 400;
uint8_t cfg_ib_device_port = 1;
unsigned int cfg_listen_port = 18515;

uint32_t cfg_cq_len = 928;       // 512
uint32_t cfg_max_send_wr = 512;  // 512
uint32_t cfg_max_recv_wr = 512;  // 512
uint32_t cfg_max_send_sge = 32;  // 32, 这个设置越小，延迟就越小
uint32_t cfg_max_recv_sge = 1;  // 32
uint32_t cfg_max_inline_data = 1024;
enum ibv_mtu cfg_mtu_enum = IBV_MTU_1024;
uint8_t cfg_max_dest_rd_atomic = 1;  // 自身作为目标的最大RDMA Reads & atomic操作数
int cfg_max_rd_atomic = 1;       // 自身作为发起者的最大RDMA Reads & atomic操作数
// unsigned int cfg_send_flags = IBV_SEND_SIGNALED | IBV_SEND_INLINE;
unsigned int cfg_send_flags = IBV_SEND_SIGNALED;

bool cfg_use_event = false;
uint8_t cfg_sl = 0;

int global_page_size = 4096;

struct my_context_t {
    struct ibv_context *ibv_ctx_;
    struct ibv_comp_channel *ibv_comp_channel_;
    struct ibv_pd *ibv_pd_;
    struct ibv_mr *ibv_mr_;
    struct ibv_cq *ibv_cq_;
    struct ibv_qp *ibv_qp_;
    struct ibv_qp_init_attr ibv_qp_init_attr_;
    struct ibv_port_attr portinfo;

    char *buf;
    uint32_t buf_size;
    unsigned int send_flags;
    int pending;  // 标记将要完成的事情
};

const char *ibv_qp_state_str(enum ibv_qp_state qp_state) {
    switch (qp_state) {
        case IBV_QPS_RESET:
            return "IBV_QPS_RESET";
            break;
        case IBV_QPS_INIT:
            return "IBV_QPS_INIT";
            break;
        case IBV_QPS_RTR:
            return "IBV_QPS_RTR";
            break;
        case IBV_QPS_RTS:
            return "IBV_QPS_RTS";
            break;
        case IBV_QPS_SQD:
            return "IBV_QPS_SQD";
            break;
        case IBV_QPS_SQE:
            return "IBV_QPS_SQE";
            break;
        case IBV_QPS_ERR:
            return "IBV_QPS_ERR";
            break;
        case IBV_QPS_UNKNOWN:
            return "IBV_QPS_UNKNOWN";
            break;

        default:
            break;
    }
    return nullptr;
}

int ibv_mtu_enum_to_value(enum ibv_mtu mtu) {
    switch (mtu) {
        case IBV_MTU_256:
            return 256;
            break;
        case IBV_MTU_512:
            return 512;
            break;
        case IBV_MTU_1024:
            return 1024;
            break;
        case IBV_MTU_2048:
            return 2048;
            break;
        case IBV_MTU_4096:
            return 4096;
            break;

        default:
            break;
    }
    return -1;
}

void print_qp_attr(struct ibv_qp_attr *attr, struct ibv_qp_init_attr *init_attr) {
    if (attr == nullptr) {
        if (init_attr == nullptr) return;
        printf("%-16s\tibv_qp_init_attr\n", "item");
        printf("%-16s\t----------------\n", "----------------");
        printf("%-16s\t%d\n", "max_send_wr", init_attr->cap.max_send_wr);
        printf("%-16s\t%d\n", "max_send_sge", init_attr->cap.max_send_sge);
        printf("%-16s\t%d\n", "max_recv_wr", init_attr->cap.max_recv_wr);
        printf("%-16s\t%d\n", "max_recv_sge", init_attr->cap.max_recv_sge);
        printf("%-16s\t%d\n", "max_inline_data", init_attr->cap.max_inline_data);
        printf("-------------------------------------------\n");
        return;
    }
    printf(
        "ibv_qp_attr:\n"
        "-------------------\n"
        "qp_state: %s\n"
        "cur_qp_state: %s\n"
        "path_mtu: %d\n"
        "rq_psn: %u\n"
        "sq_psn: %u\n"
        "max_rd_atomic: %u\n"
        "max_dest_rd_atomic: %u\n"
        "port_num: %u\n"
        "retry_cnt: %u\n"
        "sq_sig_all: %d\n"
        "-------------------\n",
        ibv_qp_state_str(attr->qp_state), ibv_qp_state_str(attr->cur_qp_state), attr->path_mtu,
        attr->rq_psn, attr->sq_psn, attr->max_rd_atomic, attr->max_dest_rd_atomic, attr->port_num,
        attr->retry_cnt, init_attr->sq_sig_all);
    printf("%-16s\t%-16s\tibv_qp_init_attr\n", "item", "ibv_qp_attr");
    printf("%-16s\t%-16s\t----------------\n", "----------------", "----------------");
    printf("%-16s\t%16d\t%d\n", "max_send_wr", attr->cap.max_send_wr, init_attr->cap.max_send_wr);
    printf("%-16s\t%16d\t%d\n", "max_send_sge", attr->cap.max_send_sge,
           init_attr->cap.max_send_sge);
    printf("%-16s\t%16d\t%d\n", "max_recv_wr", attr->cap.max_recv_wr, init_attr->cap.max_recv_wr);
    printf("%-16s\t%16d\t%d\n", "max_recv_sge", attr->cap.max_recv_sge,
           init_attr->cap.max_recv_sge);
    printf("%-16s\t%16d\t%d\n", "max_inline_data", attr->cap.max_inline_data,
           init_attr->cap.max_inline_data);
    printf("-----------------------------------------------------------------\n");
}

// 使用第一个设备
bool my_ctx_init(struct my_context_t *ctx, int buf_size, uint8_t port, bool use_event,
                 uint32_t max_inline_data = cfg_max_inline_data) {
    int access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;

    ctx->buf_size = buf_size;
    ctx->send_flags = cfg_send_flags;

    global_page_size = sysconf(_SC_PAGESIZE);
    ctx->buf = (char *)memalign(global_page_size, buf_size);
    if (!ctx->buf) {
        fprintf(stderr, "Couldn't allocate work buf.\n");
        return false;
    }

    int num_devices;
    auto device_list = ibv_get_device_list(&num_devices);
    if (device_list == nullptr) {
        perror("[ibv_get_device_list]");
        goto clean_buffer;
    }
    if (num_devices == 0) {
        fprintf(stderr, "No Device\n");
        goto clean_device_list;
    }
    printf("    %-16s\t      GUID\n", "device");
    printf("    %-16s\t----------------\n", "--------");
    for (int i = 0; i < num_devices; i++) {
        printf("    %-16s\t%016llx\n", ibv_get_device_name(device_list[i]),
               (unsigned long long)be64toh(ibv_get_device_guid(device_list[i])));
    }
    printf("----------------------------------------\n");

    ctx->ibv_ctx_ = ibv_open_device(device_list[0]);
    if (ctx->ibv_ctx_ == nullptr) {
        fprintf(stderr, "Couldn't get context for %s\n", ibv_get_device_name(device_list[0]));
        goto clean_device_list;
    }

    if (use_event) {
        ctx->ibv_comp_channel_ = ibv_create_comp_channel(ctx->ibv_ctx_);
        if (ctx->ibv_comp_channel_ == nullptr) {
            fprintf(stderr, "Couldn't create completion channel\n");
            goto clean_device;
        }
    }

    ctx->ibv_pd_ = ibv_alloc_pd(ctx->ibv_ctx_);
    if (ctx->ibv_pd_ == nullptr) {
        fprintf(stderr, "ibv_alloc_pd fail\n");
        goto clean_comp_channel;
    }

    ctx->ibv_mr_ = ibv_reg_mr(ctx->ibv_pd_, ctx->buf, buf_size, access_flags);
    if (!ctx->ibv_mr_) {
        fprintf(stderr, "Couldn't register MR\n");
        goto clean_pd;
    }

    ctx->ibv_cq_ = ibv_create_cq(ctx->ibv_ctx_, cfg_cq_len, (void *)test_cq_context,
                                 ctx->ibv_comp_channel_, 0);
    if (!ctx->ibv_cq_) {
        fprintf(stderr, "Couldn't create CQ\n");
        goto clean_mr;
    }

    {
        struct ibv_qp_attr attr;
        struct ibv_qp_init_attr init_attr = {.send_cq = ctx->ibv_cq_,
                                             .recv_cq = ctx->ibv_cq_,
                                             .cap =
                                                 {
                                                     .max_send_wr = cfg_max_send_wr,
                                                     .max_recv_wr = cfg_max_recv_wr,
                                                     .max_send_sge = cfg_max_send_sge,
                                                     .max_recv_sge = cfg_max_recv_sge,
                                                 },  // .max_inline_data = max_inline_data
                                             .qp_type = IBV_QPT_RC,
                                             .sq_sig_all = 0};
        // do{
        //     ctx->ibv_qp_ = ibv_create_qp(ctx->ibv_pd_, &init_attr);
        //     printf("max_inline_data: %u\n", init_attr.cap.max_inline_data);
        //     --init_attr.cap.max_inline_data;
        // } while(!ctx->ibv_qp_);
        ctx->ibv_qp_ = ibv_create_qp(ctx->ibv_pd_, &init_attr);
        if (!ctx->ibv_qp_) {
            fprintf(stderr, "Couldn't create QP\n");
            goto clean_cq;
        }
        print_qp_attr(nullptr, &init_attr);
    }

    {
        struct ibv_qp_attr attr = {
            .qp_state = IBV_QPS_INIT,
            .qp_access_flags = 0,
            .pkey_index = 0,
            .port_num = port,

        };

        if (ibv_modify_qp(ctx->ibv_qp_, &attr,
                          IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS)) {
            fprintf(stderr, "Failed to modify QP to INIT\n");
            goto clean_qp;
        }

        int attr_mask = IBV_QP_STATE | IBV_QP_CUR_STATE | IBV_QP_ACCESS_FLAGS | IBV_QP_PORT |
                        IBV_QP_PATH_MTU | IBV_QP_RETRY_CNT | IBV_QP_RQ_PSN |
                        IBV_QP_MAX_QP_RD_ATOMIC | IBV_QP_SQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
                        IBV_QP_CAP;
        errno = ibv_query_qp(ctx->ibv_qp_, &attr, attr_mask, &ctx->ibv_qp_init_attr_);
        if (errno == 0) {
            print_qp_attr(&attr, &ctx->ibv_qp_init_attr_);
            // if (ctx->ibv_qp_init_attr_.cap.max_inline_data >= buf_size) {
            //     ctx->send_flags |= IBV_SEND_INLINE;
            //     printf("IBV_SEND_INLINE------\n");
            // }
        } else {
            perror("ibv_query_qp");
        }
    }

    ibv_free_device_list(device_list);
    return true;

clean_qp:
    errno = ibv_destroy_qp(ctx->ibv_qp_);
    if (errno) {
        perror("ibv_destroy_qp");
    }

clean_cq:
    errno = ibv_destroy_cq(ctx->ibv_cq_);
    if (errno) {
        perror("ibv_destroy_cq");
    }

clean_mr:
    errno = ibv_dereg_mr(ctx->ibv_mr_);
    if (errno) {
        perror("ibv_dereg_mr");
    }

clean_pd:
    errno = ibv_dealloc_pd(ctx->ibv_pd_);
    if (errno) {
        perror("ibv_dealloc_pd");
    }

clean_comp_channel:
    if (ctx->ibv_comp_channel_) {
        errno = ibv_destroy_comp_channel(ctx->ibv_comp_channel_);
        if (errno) {
            perror("[ibv_destroy_comp_channel]");
        }
    }

clean_device:
    errno = ibv_close_device(ctx->ibv_ctx_);
    if (errno) {
        perror("[ibv_close_device]");
    }

clean_device_list:
    ibv_free_device_list(device_list);

clean_buffer:
    free(ctx->buf);
    return false;
}

void my_ctx_destory(struct my_context_t *ctx) {
    errno = ibv_destroy_qp(ctx->ibv_qp_);
    if (errno) {
        perror("ibv_destroy_qp");
    }

    errno = ibv_destroy_cq(ctx->ibv_cq_);
    if (errno) {
        perror("ibv_destroy_cq");
    }

    errno = ibv_dereg_mr(ctx->ibv_mr_);
    if (errno) {
        perror("ibv_dereg_mr");
    }

    errno = ibv_dealloc_pd(ctx->ibv_pd_);
    if (errno) {
        perror("ibv_dealloc_pd");
    }

    if (ctx->ibv_comp_channel_) {
        errno = ibv_destroy_comp_channel(ctx->ibv_comp_channel_);
        if (errno) {
            perror("[ibv_destroy_comp_channel]");
        }
    }

    errno = ibv_close_device(ctx->ibv_ctx_);
    if (errno) {
        perror("[ibv_close_device]");
    }

    free(ctx->buf);
}

enum {
    PINGPONG_RECV_WRID = 1,
    PINGPONG_SEND_WRID = 2,
};

static int pp_post_recv(struct my_context_t *ctx, int n) {
    struct ibv_sge list = {
        .addr = (uintptr_t)ctx->buf, .length = ctx->buf_size, .lkey = ctx->ibv_mr_->lkey};
    struct ibv_recv_wr wr = {
        .wr_id = PINGPONG_RECV_WRID,
        .sg_list = &list,
        .num_sge = 1,
    };
    struct ibv_recv_wr *bad_wr;
    int i;

    for (i = 0; i < n; ++i)
        if (ibv_post_recv(ctx->ibv_qp_, &wr, &bad_wr)) break;

    return i;
}

static int pp_post_send(struct my_context_t *ctx) {
    struct ibv_sge list = {
        .addr = (uintptr_t)ctx->buf, .length = ctx->buf_size, .lkey = ctx->ibv_mr_->lkey};
    struct ibv_send_wr wr = {
        .wr_id = PINGPONG_SEND_WRID,
        .sg_list = &list,
        .num_sge = 1,
        .opcode = IBV_WR_SEND,
        .send_flags = ctx->send_flags,
    };
    struct ibv_send_wr *bad_wr;

    return ibv_post_send(ctx->ibv_qp_, &wr, &bad_wr);
}

// ibv_devinfo -v -d ibp102s0 -i 1
void print_ibv_port_attr(struct ibv_port_attr &portinfo) {
    printf("print_ibv_port_attr:\n");
    printf("%-16s\t----------------\n", "----------------");
    printf("%-16s\t%d\n", "max_mtu", portinfo.max_mtu);
    printf("%-16s\t%d\n", "active_mtu", portinfo.active_mtu);
    printf("%-16s\t%d\n", "lid", portinfo.lid);
    printf("%-16s\t%d\n", "active_width", portinfo.active_width);
    printf("%-16s\t%d\n", "active_speed", portinfo.active_speed);
    printf("%-16s\t%d\n", "link_layer", portinfo.link_layer);
    printf("-------------------------------------------\n");
    return;
}

struct pingpong_dest {
    uint16_t lid;
    uint32_t qpn;
    uint32_t psn;
    union ibv_gid gid;  // 暂时不明确用法
};

void wire_gid_to_gid(const char *wgid, union ibv_gid *gid) {
    char tmp[9];
    __be32 v32;
    int i;
    uint32_t tmp_gid[4];

    for (tmp[8] = 0, i = 0; i < 4; ++i) {
        memcpy(tmp, wgid + i * 8, 8);
        sscanf(tmp, "%x", &v32);
        tmp_gid[i] = be32toh(v32);
    }
    memcpy(gid, tmp_gid, sizeof(*gid));
}

void gid_to_wire_gid(const union ibv_gid *gid, char wgid[]) {
    uint32_t tmp_gid[4];
    int i;

    memcpy(tmp_gid, gid, sizeof(tmp_gid));
    for (i = 0; i < 4; ++i) sprintf(&wgid[i * 8], "%08x", htobe32(tmp_gid[i]));
}

static int pp_connect_ctx(struct my_context_t *ctx, uint8_t port, int my_psn, enum ibv_mtu mtu, uint8_t sl,
                          struct pingpong_dest *dest, int sgid_idx) {
    struct ibv_qp_attr attr = {
        .qp_state = IBV_QPS_RTR,
        .path_mtu = mtu,
        .rq_psn = dest->psn,  // rq的psn需要跟远端的psn对应
        .dest_qp_num = dest->qpn,
        .ah_attr = {
            .dlid = dest->lid, .sl = sl, .src_path_bits = 0, .is_global = 0, .port_num = port},
        .max_dest_rd_atomic = cfg_max_dest_rd_atomic,
        .min_rnr_timer = 12  // Minimum RNR NAK timer (valid only for RC QPs)
                              // 接受到请求，但RQ中没有对应WR时的重试次数，应该是本地重试吧
        };
    
    if (dest->gid.global.interface_id) {
        attr.ah_attr.is_global = 1;
        attr.ah_attr.grh.hop_limit = 1;  // 设置为1确保不会离开子网
        attr.ah_attr.grh.dgid = dest->gid;
        attr.ah_attr.grh.sgid_index = sgid_idx;
        attr.ah_attr.grh.flow_label = 0;
    }
    if (ibv_modify_qp(ctx->ibv_qp_, &attr,
                      IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                          IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER)) {
        fprintf(stderr, "Failed to modify QP to RTR\n");
        return 1;
    }

    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 14;     // 0.0671 sec, 等待回复的超时时间
    attr.retry_cnt = 7;    // 远端没有应答时的重试次数
    attr.rnr_retry = 7;    // 接受到 RNR NACK 时的重试次数，7表示无限重试
    attr.sq_psn = my_psn;  // 自身生成的psn
    attr.max_rd_atomic = cfg_max_rd_atomic;
    if (ibv_modify_qp(ctx->ibv_qp_, &attr,
                      IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY |
                          IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC)) {
        fprintf(stderr, "Failed to modify QP to RTS\n");
        return 1;
    }

    return 0;
}

static struct pingpong_dest *pp_client_exch_dest(const char *servername, int port,
                                                 const struct pingpong_dest *my_dest) {
    struct addrinfo *res, *t;
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    char *service;
    char msg[sizeof "0000:000000:000000:00000000000000000000000000000000"];
    int n;
    int sockfd = -1;
    struct pingpong_dest *rem_dest = NULL;
    char gid[33];

    if (asprintf(&service, "%d", port) < 0) return NULL;

    n = getaddrinfo(servername, service, &hints, &res);

    if (n < 0) {
        fprintf(stderr, "%s for %s:%d\n", gai_strerror(n), servername, port);
        free(service);
        return NULL;
    }

    for (t = res; t; t = t->ai_next) {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {
            if (!connect(sockfd, t->ai_addr, t->ai_addrlen))  // 连接成功了直接退出
                break;
            close(sockfd);
            sockfd = -1;
        }
    }

    freeaddrinfo(res);
    free(service);

    if (sockfd < 0) {
        fprintf(stderr, "Couldn't connect to %s:%d\n", servername, port);
        return NULL;
    }

    gid_to_wire_gid(&my_dest->gid, gid);
    sprintf(msg, "%04x:%06x:%06x:%s", my_dest->lid, my_dest->qpn, my_dest->psn, gid);
    if (write(sockfd, msg, sizeof msg) != sizeof msg) {
        fprintf(stderr, "Couldn't send local address\n");
        goto out;
    }

    if (read(sockfd, msg, sizeof msg) != sizeof msg ||
        write(sockfd, "done", sizeof "done") != sizeof "done") {
        perror("client read/write");
        fprintf(stderr, "Couldn't read/write remote address\n");
        goto out;
    }

    rem_dest = (struct pingpong_dest *)malloc(sizeof *rem_dest);
    if (!rem_dest) goto out;

    sscanf(msg, "%hx:%x:%x:%s", &rem_dest->lid, &rem_dest->qpn, &rem_dest->psn, gid);
    wire_gid_to_gid(gid, &rem_dest->gid);

out:
    close(sockfd);
    return rem_dest;
}

static struct pingpong_dest *pp_server_exch_dest(struct my_context_t *ctx, int ib_port,
                                                 enum ibv_mtu mtu, int port, int sl,
                                                 const struct pingpong_dest *my_dest,
                                                 int sgid_idx) {
    struct addrinfo *res, *t;
    struct addrinfo hints = {
        .ai_flags = AI_PASSIVE, .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    char *service;
    char msg[sizeof "0000:000000:000000:00000000000000000000000000000000"];
    int n;
    int sockfd = -1, connfd;
    struct pingpong_dest *rem_dest = NULL;
    char gid[33];

    // 内部分配空间
    if (asprintf(&service, "%d", port) < 0) return NULL;
    // service 指定端口号
    // node 指定为NULL，表示用于本地通信的本地地址
    n = getaddrinfo(NULL, service, &hints, &res);

    if (n < 0) {
        fprintf(stderr, "%s for port %d\n", gai_strerror(n), port);
        free(service);
        return NULL;
    }

    for (t = res; t; t = t->ai_next) {
        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {
            n = 1;

            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);

            if (!bind(sockfd, t->ai_addr, t->ai_addrlen)) break;
            close(sockfd);
            sockfd = -1;
        }
    }

    freeaddrinfo(res);
    free(service);

    if (sockfd < 0) {
        fprintf(stderr, "Couldn't listen to port %d\n", port);
        return NULL;
    }

    listen(sockfd, 1);
    connfd = accept(sockfd, NULL, NULL);
    close(sockfd);
    if (connfd < 0) {
        fprintf(stderr, "accept() failed\n");
        return NULL;
    }

    n = read(connfd, msg, sizeof msg);
    if (n != sizeof msg) {
        perror("server read");
        fprintf(stderr, "%d/%d: Couldn't read remote address\n", n, (int)sizeof msg);
        goto out;
    }

    rem_dest = (struct pingpong_dest *)malloc(sizeof *rem_dest);
    if (!rem_dest) goto out;

    sscanf(msg, "%hx:%x:%x:%s", &rem_dest->lid, &rem_dest->qpn, &rem_dest->psn, gid);
    wire_gid_to_gid(gid, &rem_dest->gid);

    if (pp_connect_ctx(ctx, ib_port, my_dest->psn, mtu, sl, rem_dest, sgid_idx)) {
        fprintf(stderr, "Couldn't connect to remote QP\n");
        free(rem_dest);
        rem_dest = NULL;
        goto out;
    }

    gid_to_wire_gid(&my_dest->gid, gid);
    // 把自身的地址返回
    sprintf(msg, "%04x:%06x:%06x:%s", my_dest->lid, my_dest->qpn, my_dest->psn, gid);
    if (write(connfd, msg, sizeof msg) != sizeof msg ||
        read(connfd, msg, sizeof msg) != sizeof "done") {
        fprintf(stderr, "Couldn't send/recv local address\n");
        free(rem_dest);
        rem_dest = NULL;
        goto out;
    }

out:
    close(connfd);
    return rem_dest;
}

static inline int parse_single_wc(struct my_context_t *ctx, int *scnt, int *rcnt, int *routs,
                                  int iters, uint64_t wr_id, enum ibv_wc_status status) {
    if (status != IBV_WC_SUCCESS) {
        fprintf(stderr, "Failed status %s (%d) for wr_id %d\n", ibv_wc_status_str(status), status,
                (int)wr_id);
        return 1;
    }

    switch ((int)wr_id) {
        case PINGPONG_SEND_WRID:
            ++(*scnt);
            break;

        case PINGPONG_RECV_WRID:
            if (--(*routs) <= 1) {
                *routs += pp_post_recv(ctx, cfg_max_recv_wr - *routs);
                if (*routs < cfg_max_recv_wr) {
                    fprintf(stderr, "Couldn't post receive (%d)\n", *routs);
                    return 1;
                }
            }

            ++(*rcnt);

            break;

        default:
            fprintf(stderr, "Completion for unknown wr_id %d\n", (int)wr_id);
            return 1;
    }

    ctx->pending &= ~(int)wr_id;
    if (*scnt < iters && !ctx->pending) {
        if (pp_post_send(ctx)) {
            fprintf(stderr, "Couldn't post send\n");
            return 1;
        }
        ctx->pending = PINGPONG_RECV_WRID | PINGPONG_SEND_WRID;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    bool ret = false;
    int gidx = -1;
    char gid[33];

    my_context_t ctx;
    ret = my_ctx_init(&ctx, cfg_buf_size, cfg_ib_device_port, cfg_use_event, cfg_max_inline_data);
    assert(ret);

    // 预先 post 足够多的 wr
    int routs = pp_post_recv(&ctx, cfg_max_recv_wr);
    if (routs < cfg_max_recv_wr) {
        fprintf(stderr, "Couldn't post receive (%d)\n", routs);
        return 1;
    }

    if (cfg_use_event)
        if (ibv_req_notify_cq(ctx.ibv_cq_, 0)) {
            fprintf(stderr, "Couldn't request CQ notification\n");
            return 1;
        }

    if (errno = ibv_query_port(ctx.ibv_ctx_, cfg_ib_device_port, &ctx.portinfo)) {
        perror("[ibv_query_port]");
        return 1;
    }
    print_ibv_port_attr(ctx.portinfo);
    if (ctx.portinfo.link_layer != IBV_LINK_LAYER_ETHERNET && !ctx.portinfo.lid) {
        fprintf(stderr, "Couldn't get local LID\n");
        return 1;
    }

    struct pingpong_dest my_dest;
    if (gidx >= 0) {
        if (ibv_query_gid(ctx.ibv_ctx_, cfg_ib_device_port, gidx, &my_dest.gid)) {
            fprintf(stderr, "can't read sgid of index %d\n", gidx);
            return 1;
        }
    } else
        memset(&my_dest.gid, 0, sizeof my_dest.gid);

    srand48(getpid() * time(NULL));
    my_dest.lid = ctx.portinfo.lid;
    my_dest.qpn = ctx.ibv_qp_->qp_num;
    my_dest.psn = lrand48() & 0xffffff;
    inet_ntop(AF_INET6, &my_dest.gid, gid, sizeof gid);
    printf("  local address:  LID 0x%04x, QPN 0x%06x, PSN 0x%06x, GID %s\n", my_dest.lid,
           my_dest.qpn, my_dest.psn, gid);

    char *servername = NULL;
    if (argc >= 2) {
        servername = strdupa(argv[argc - 1]);
    }

    struct pingpong_dest *rem_dest;
    if (servername)
        rem_dest = pp_client_exch_dest(servername, cfg_listen_port, &my_dest);
    else {
        rem_dest = pp_server_exch_dest(&ctx, cfg_ib_device_port, cfg_mtu_enum, cfg_listen_port,
                                       cfg_sl, &my_dest, gidx);
    }
    if (!rem_dest) return 1;

    inet_ntop(AF_INET6, &rem_dest->gid, gid, sizeof gid);
    printf("  remote address: LID 0x%04x, QPN 0x%06x, PSN 0x%06x, GID %s\n", rem_dest->lid,
           rem_dest->qpn, rem_dest->psn, gid);

    if (servername)
        if (pp_connect_ctx(&ctx, cfg_ib_device_port, my_dest.psn, cfg_mtu_enum, cfg_sl, rem_dest,
                           gidx))
            return 1;
    ctx.pending = PINGPONG_RECV_WRID;

    if (servername) {
        for (int i = 0; i < cfg_buf_size; i += global_page_size) {
            ctx.buf[i] = i / global_page_size % sizeof(char);
        }

        if (pp_post_send(&ctx)) {
            fprintf(stderr, "Couldn't post send\n");
            return 1;
        }
        ctx.pending |= PINGPONG_SEND_WRID;
    }

    int rcnt = 0, scnt = 0;
    int num_cq_events = 0;
    struct timeval start, end;
    if (gettimeofday(&start, NULL)) {
        perror("gettimeofday");
        return 1;
    }

    while (rcnt < cfg_iters || scnt < cfg_iters) {
        int ret;

        if (cfg_use_event) {
            struct ibv_cq *ev_cq;
            void *ev_ctx;

            if (ibv_get_cq_event(ctx.ibv_comp_channel_, &ev_cq, &ev_ctx)) {
                fprintf(stderr, "Failed to get cq_event\n");
                return 1;
            }

            ++num_cq_events;

            if (ev_cq != ctx.ibv_cq_) {
                fprintf(stderr, "CQ event for unknown CQ %p\n", ev_cq);
                return 1;
            }
            // assert(ev_ctx == ev_cq->cq_context);
            // assert((uintptr_t)ev_ctx == test_cq_context);

            if (ibv_req_notify_cq(ctx.ibv_cq_, 0)) {
                fprintf(stderr, "Couldn't request CQ notification\n");
                return 1;
            }
        }

        int ne, i;
        struct ibv_wc wc[2];  // 因为最多的情况会有两个完成元素，但只有一个时也会正常处理

        do {
            ne = ibv_poll_cq(ctx.ibv_cq_, 2, wc);
            if (ne < 0) {
                fprintf(stderr, "poll CQ failed %d\n", ne);
                return 1;
            }
        } while (!cfg_use_event && ne < 1);  // 如果使用事件 ne 可能为0

        for (i = 0; i < ne; ++i) {
            ret = parse_single_wc(&ctx, &scnt, &rcnt, &routs, cfg_iters, wc[i].wr_id, wc[i].status);
            if (ret) {
                fprintf(stderr, "parse WC failed %d\n", ne);
                return 1;
            }
        }
    }

    if (gettimeofday(&end, NULL)) {
        perror("gettimeofday");
        return 1;
    }

    {
		float usec = (end.tv_sec - start.tv_sec) * 1000000 +
			(end.tv_usec - start.tv_usec);
		long long bytes = (long long) cfg_buf_size * cfg_iters * 2;

		printf("%lld bytes in %.2f seconds = %.2f Mbit/sec\n",
		       bytes, usec / 1000000., bytes * 8. / usec);
		printf("%d iters in %.2f seconds = %.2f usec/iter\n",
		       cfg_iters, usec / 1000000., usec / cfg_iters);

		if ((!servername)) {
			for (int i = 0; i < cfg_buf_size; i += global_page_size)
				if (ctx.buf[i] != i / global_page_size % sizeof(char))
					printf("invalid data in page %d\n",
					       i / global_page_size);
		}
	}

	ibv_ack_cq_events(ctx.ibv_cq_, num_cq_events);

    my_ctx_destory(&ctx);
    // if (servername) {
    //     free(servername);
    // }
    free(rem_dest);
    return 0;
}