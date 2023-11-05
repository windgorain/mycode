/*================================================================
*   Created by LiXingang
*   Description:
*
================================================================*/
#include "bs.h"
#include "utl/afxdp_utl.h"
#include "utl/free_list.h"

#ifdef IN_LINUX

#include <sys/mman.h>
#include <linux/bpf.h>
#include <linux/if_link.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/limits.h>
#include <net/if.h>
#include <bpf/libbpf.h>
#include <bpf/xsk.h>
#include <bpf/bpf.h>

typedef struct
{
    void *bufs;
    int buf_size;
    int if_index;
    char if_name[16];
    struct xsk_umem_info *umem;
    struct xsk_socket_info *xsk;
    struct xsk_prog_info *prog;
    FREE_LIST_S free_list;
} AFXDP_CTRL_S;

struct xsk_umem_info
{
    struct xsk_ring_prod fq;
    struct xsk_ring_cons cq;
    struct xsk_umem *umem;
    void *buffer;
};

struct xsk_ring_stats
{
    unsigned long rx_npkts;
    unsigned long tx_npkts;
    unsigned long rx_dropped_npkts;
    unsigned long rx_invalid_npkts;
    unsigned long tx_invalid_npkts;
    unsigned long rx_full_npkts;
    unsigned long rx_fill_empty_npkts;
    unsigned long tx_empty_npkts;
    unsigned long prev_rx_npkts;
    unsigned long prev_tx_npkts;
    unsigned long prev_rx_dropped_npkts;
    unsigned long prev_rx_invalid_npkts;
    unsigned long prev_tx_invalid_npkts;
    unsigned long prev_rx_full_npkts;
    unsigned long prev_rx_fill_empty_npkts;
    unsigned long prev_tx_empty_npkts;
};

struct xsk_socket_info
{
    struct xsk_ring_cons rx;
    struct xsk_ring_prod tx;
    struct xsk_umem_info *umem;
    struct xsk_socket *xsk;
    struct xsk_ring_stats ring_stats;
    UINT outstanding_tx;
};

struct xsk_prog_info {
    struct bpf_object *obj;
    int xsks_map_fd;
    UINT xdp_flags;
};

static AFXDP_PARAM_S g_afxdp_dft_param = {
    .frame_size = XSK_UMEM__DEFAULT_FRAME_SIZE,
    .frame_count = 4096,
    .fq_size = 4096,
    .cq_size = 2048,
    .rx_size = 2048,
    .tx_size = 2048,
    .xsk_num = 1,
    .head_room = XSK_UMEM__DEFAULT_FRAME_HEADROOM,
    .flag = AFXDP_FLAG_RX,
    .libbpf_flags = 0,
    .xdp_flags = 0,
    .bind_flags = 0};

static struct xsk_umem_info *afxdp_configure_umem(void *buffer, UINT size, AFXDP_PARAM_S *p)
{
    int ret;
    struct xsk_umem_info *umem;
    struct xsk_umem_config cfg = {
        .fill_size = p->fq_size,
        .comp_size = p->cq_size,
        .frame_size = p->frame_size,
        .frame_headroom = p->head_room,
        .flags = 0};

    umem = MEM_ZMalloc(sizeof(struct xsk_umem_info));
    if (!umem)
    {
        return NULL;
    }

    ret = xsk_umem__create(&umem->umem, buffer, size, &umem->fq, &umem->cq, &cfg);
    if (ret)
    {
        MEM_Free(umem);
        return NULL;
    }

    umem->buffer = buffer;

    return umem;
}

static int afxdp_populate_fill_ring(AFXDP_CTRL_S *ctrl, AFXDP_PARAM_S *p)
{
    int ret, i;
    UINT idx;
    char *node;

    ret = xsk_ring_prod__reserve(&ctrl->umem->fq, p->fq_size, &idx);
    if (ret != p->fq_size)
    {
        RETURN(BS_ERR);
    }

    for (i = 0; i < p->fq_size; i++)
    {
        node = FreeList_Get(&ctrl->free_list);
        if (!node)
        {
            BS_DBGASSERT(0);
            RETURN(BS_ERR);
        }

        *xsk_ring_prod__fill_addr(&ctrl->umem->fq, idx++) = node - (char *)ctrl->bufs;
    }

    xsk_ring_prod__submit(&ctrl->umem->fq, p->fq_size);

    return 0;
}

static struct xsk_socket_info *afxdp_configure_socket(struct xsk_umem_info *umem, char *if_name, AFXDP_PARAM_S *p)
{
    struct xsk_socket_config cfg;
    struct xsk_socket_info *xsk;
    struct xsk_ring_cons *rxr;
    struct xsk_ring_prod *txr;
    int ret;
    UINT prog_id;
    UINT opt_xdp_flags = p->xdp_flags;

    xsk = MEM_ZMalloc(sizeof(*xsk));
    if (!xsk)
    {
        return NULL;
    }

    xsk->umem = umem;
    cfg.rx_size = p->rx_size;
    cfg.tx_size = p->tx_size;
    cfg.libbpf_flags = p->libbpf_flags;
    cfg.xdp_flags = opt_xdp_flags;
    cfg.bind_flags = p->bind_flags;

    rxr = (p->flag & AFXDP_FLAG_RX) ? &xsk->rx : NULL;
    txr = (p->flag & AFXDP_FLAG_TX) ? &xsk->tx : NULL;
    ret = xsk_socket__create(&xsk->xsk, if_name, 0, umem->umem, rxr, txr, &cfg);
    if (ret)
    {
        MEM_Free(xsk);
        return NULL;
    }

    ret = bpf_xdp_query_id(if_nametoindex(if_name), opt_xdp_flags, &prog_id);
    if (ret)
    {
        MEM_Free(xsk);
        return NULL;
    }

    return xsk;
}

static void kick_tx(struct xsk_socket_info *xsk)
{
    sendto(xsk_socket__fd(xsk->xsk), NULL, 0, MSG_DONTWAIT, NULL, 0);
}

static inline void complete_tx_only(struct xsk_socket_info *xsk, int batch_size)
{
    unsigned int rcvd;
    UINT idx;

    if (!xsk->outstanding_tx)
        return;

    if (xsk_ring_prod__needs_wakeup(&xsk->tx))
    {
        kick_tx(xsk);
    }

    rcvd = xsk_ring_cons__peek(&xsk->umem->cq, batch_size, &idx);
    if (rcvd > 0)
    {
        xsk_ring_cons__release(&xsk->umem->cq, rcvd);
        xsk->outstanding_tx -= rcvd;
        xsk->ring_stats.tx_npkts += rcvd;
    }
}

static int afxdp_SendPackets(struct xsk_socket_info *xsk, LDATA_S *pkts, int batch_size)
{
    UINT idx;
    unsigned int i;

    while (xsk_ring_prod__reserve(&xsk->tx, batch_size, &idx) < batch_size)
    {
        RETURN(BS_NO_RESOURCE);
    }

    for (i = 0; i < batch_size; i++)
    {
        struct xdp_desc *tx_desc = xsk_ring_prod__tx_desc(&xsk->tx, idx + i);
        tx_desc->addr = (UINT64)pkts[i].data;
        tx_desc->len = pkts[i].len;
    }

    xsk_ring_prod__submit(&xsk->tx, batch_size);
    xsk->outstanding_tx += batch_size;
    complete_tx_only(xsk, batch_size);

    return 0;
}

static void afxdp_init_free_list(AFXDP_CTRL_S *ctrl, AFXDP_PARAM_S *p)
{
    FreeList_Init(&ctrl->free_list);
    FreeList_Puts(&ctrl->free_list, ctrl->bufs, p->frame_size, p->frame_count);
}

static int afxdp_load_program(AFXDP_CTRL_S *ctrl, AFXDP_PARAM_S *p, CHAR *file_path)
{
    char xdp_filename[256];
    int prog_fd;
    struct bpf_object *obj = NULL;
    struct xsk_prog_info *prog_info = NULL;
    struct bpf_program *prog;
    struct bpf_map *map;
    int xsks_map_fd;

    if (ctrl->prog)
    {
        fprintf(stderr, "ERROR: need to remove program first\n");
        return BS_ERR;
    }

    prog_info = MEM_ZMalloc(sizeof(struct xsk_prog_info));
    if (!prog_info) {
        return BS_NO_MEMORY;
    }

    snprintf(xdp_filename, sizeof(xdp_filename), "%s", file_path);

    obj = bpf_object__open(xdp_filename);
    if (! obj) {
        fprintf(stderr, "ERROR: Can't open %s \n", xdp_filename);
        goto OUT_ERR;
    }

    if (bpf_object__load(obj) != 0) {
        fprintf(stderr, "ERROR: Can't load obj \n");
        goto OUT_ERR;
    }

    map = bpf_object__find_map_by_name(obj, "xsks_map");
    xsks_map_fd = bpf_map__fd(map);
    if (xsks_map_fd < 0) {
        fprintf(stderr, "ERROR: no xsks map found: %s\n", strerror(xsks_map_fd));
        goto OUT_ERR;
    }

    prog = bpf_object__next_program(obj, NULL);
    if (! prog) {
        fprintf(stderr, "ERROR: Can't get prog \n");
        goto OUT_ERR;
    }

	prog_fd = bpf_program__fd(prog);
    if (bpf_xdp_attach(ctrl->if_index, prog_fd, p->xdp_flags, NULL) != 0) {
        fprintf(stderr, "ERROR: link set xdp fd failed\n");
        goto OUT_ERR;
    }

    prog_info->obj = obj;
    prog_info->xsks_map_fd = xsks_map_fd;
    prog_info->xdp_flags = p->xdp_flags;
    ctrl->prog = prog_info;

    return BS_OK;

OUT_ERR:
    if (prog_info) {
        MEM_Free(prog_info);
    }
    if (obj) {
        bpf_object__close(obj);
    }
    return BS_ERR;

}

static void afxdp_remove_program(AFXDP_CTRL_S *ctrl)
{
    if (!ctrl->prog) {
        return;
    }

    if (ctrl->prog->obj) {
        bpf_xdp_detach(ctrl->if_index, ctrl->prog->xdp_flags, NULL);
		bpf_object__close(ctrl->prog->obj);
    }

    MEM_Free(ctrl->prog);
    ctrl->prog = NULL;

    return;
}

static int afxdp_update_map(AFXDP_CTRL_S *ctrl)
{
    int ret;
    int queue_id = 0;
    int fd = xsk_socket__fd(ctrl->xsk->xsk);
    ret = bpf_map_update_elem(ctrl->prog->xsks_map_fd, &queue_id, &fd, 0);
    if (ret)
    {
        fprintf(stderr, "ERROR: bpf_map_update_elem\n");
        return BS_ERR;
    }

    return BS_OK;
}

AFXDP_HANDLE AFXDP_Create(char *if_name, char *bpf_prog, AFXDP_PARAM_S *p)
{
    AFXDP_CTRL_S *ctrl;
    int size;

    if (!p)
    {
        p = &g_afxdp_dft_param;
    }

    ctrl = MEM_ZMalloc(sizeof(AFXDP_CTRL_S));
    if (!ctrl)
    {
        return NULL;
    }
    strlcpy(ctrl->if_name, if_name, sizeof(ctrl->if_name));
    ctrl->if_index = if_nametoindex(if_name);

    if (bpf_prog) {
        if (0 != afxdp_load_program(ctrl, p, bpf_prog)) {
            AFXDP_Destroy(ctrl);
            return NULL;
        }
    }

    size = p->frame_size * p->frame_count;
    void *bufs = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (bufs == MAP_FAILED) {
        AFXDP_Destroy(ctrl);
        return NULL;
    }

    ctrl->bufs = bufs;
    ctrl->buf_size = size;

    afxdp_init_free_list(ctrl, p);

    ctrl->umem = afxdp_configure_umem(bufs, size, p);
    if (!ctrl->umem) {
        AFXDP_Destroy(ctrl);
        return NULL;
    }

    if (p->flag & AFXDP_FLAG_RX) {
        if (0 != afxdp_populate_fill_ring(ctrl, p)) {
            AFXDP_Destroy(ctrl);
            return NULL;
        }
    }

    ctrl->xsk = afxdp_configure_socket(ctrl->umem, if_name, p);
    if (!ctrl->xsk) {
        AFXDP_Destroy(ctrl);
        return NULL;
    }

    if (bpf_prog) {
        if (0 != afxdp_update_map(ctrl)) {
            AFXDP_Destroy(ctrl);
            return NULL;
        }
    }

    return ctrl;
}

void AFXDP_Destroy(AFXDP_HANDLE hAfXdp)
{
    AFXDP_CTRL_S *ctrl = hAfXdp;

    if (ctrl->xsk) {
        if (ctrl->xsk->xsk) {
            xsk_socket__delete(ctrl->xsk->xsk);
        }
        MEM_Free(ctrl->xsk);
    }

    if (ctrl->umem) {
        if (ctrl->umem->umem) {
            xsk_umem__delete(ctrl->umem->umem);
        }
        MEM_Free(ctrl->umem);
    }

    if (ctrl->bufs) {
        munmap(ctrl->bufs, ctrl->buf_size);
    }

    if (ctrl->prog) {
        afxdp_remove_program(ctrl);
    }

    MEM_Free(ctrl);
}

void AFXDP_RecvPkt(AFXDP_HANDLE hAfXdp, UINT batch_size, PF_AFXDP_RECV_PKT func, void *ud)
{
    AFXDP_CTRL_S *ctrl = hAfXdp;
    struct xsk_socket_info *xsk = ctrl->xsk;
    UINT idx_rx = 0, idx_fq = 0;
    const struct xdp_desc *desc;
    UINT i;

    UINT rcvd = xsk_cons_nb_avail(&xsk->rx, batch_size);
    if (rcvd == 0) {
        return;
    }
    UINT free_count = xsk_prod_nb_free(&xsk->umem->fq, rcvd);

    rcvd = MIN(free_count, rcvd);
    if (rcvd == 0) {
        return;
    }

    idx_rx = xsk->rx.cached_cons;
    xsk->rx.cached_cons += rcvd;
    idx_fq = xsk->umem->fq.cached_prod;
    xsk->umem->fq.cached_prod += rcvd;

    for (i = 0; i < rcvd; i++) {
        desc = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx);
        UINT64 addr = desc->addr;
        UINT len = desc->len;
        UINT64 orig = xsk_umem__extract_addr(addr);

        addr = xsk_umem__add_offset_to_addr(addr);
        void *pkt = xsk_umem__get_data(xsk->umem->buffer, addr);

        func(pkt, len, ud);

        *xsk_ring_prod__fill_addr(&xsk->umem->fq, idx_fq++) = orig;
    }

    xsk_ring_prod__submit(&xsk->umem->fq, rcvd);
    xsk_ring_cons__release(&xsk->rx, rcvd);
    xsk->ring_stats.rx_npkts += rcvd;
}

int AFXDP_SendPkt(AFXDP_HANDLE hAfXdp, void *data, int data_len)
{
    AFXDP_CTRL_S *ctrl = hAfXdp;
    LDATA_S pkt;
    void *node;

    node = FreeList_Get(&ctrl->free_list);
    if (!node) {
        RETURN(BS_NO_MEMORY);
    }

    MEM_Copy(node, data, data_len);

    pkt.data = (void *)((char *)node - (char *)ctrl->bufs);
    pkt.len = data_len;

    return afxdp_SendPackets(ctrl->xsk, &pkt, 1);
}

#endif
