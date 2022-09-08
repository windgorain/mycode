/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "comp/comp_event_hub.h"
#include "../h/ulc_def.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_prog.h"
#include "../h/ulc_bpf.h"
#include "../h/ulc_osbase.h"
#include "../h/ulc_hookpoint.h"

typedef struct {
    DLL_NODE_S link_node;
    int fd;
}ULC_HOOKPOINT_NODE_S;

static DLL_HEAD_S g_ulc_xdp_list = DLL_HEAD_INIT_VALUE(&g_ulc_xdp_list);
static MUTEX_S g_ulc_hookpoint_mutex;

static inline int ulc_hookpoint_process_xdp_fd(ULC_XDP_BUFF_S *xdp_buf, int fd)
{
    ULC_PROG_NODE_S *prog;

    prog = ULC_PROG_GetByFD(fd);
    if (! prog) {
        return XDP_PASS;
    }

    return ULC_RunBpfCode(prog->insn, (long)xdp_buf, 0, 0, 0, 0);
}

static int ulc_hookpoint_xdp_input(ULC_XDP_BUFF_S *xdp_buf)
{
    ULC_HOOKPOINT_NODE_S *node;
    int ret = XDP_PASS;

    int state = RcuEngine_Lock();

    DLL_SCAN(&g_ulc_xdp_list, node) {
        ret = ulc_hookpoint_process_xdp_fd(xdp_buf, node->fd);
        if (ret != XDP_PASS) {
            break;
        }
    }

    RcuEngine_UnLock(state);

    return ret;
}

static int ulc_hookpoint_precver_pkt_input(void *ob, void *data)
{
    PRECVER_PKT_S *pkt = data;
    ULC_XDP_BUFF_S xdp_buf = {0};

    xdp_buf.data = pkt->data;
    xdp_buf.data_end = pkt->data + pkt->data_len;

    ulc_hookpoint_xdp_input(&xdp_buf);

    return 0;
}

static ULC_HOOKPOINT_NODE_S * ulc_hookpoint_xdp_detach(int fd)
{
    ULC_HOOKPOINT_NODE_S *node;
    ULC_HOOKPOINT_NODE_S *found = NULL;

    DLL_SCAN(&g_ulc_xdp_list, node) {
        if (node->fd == fd) {
            found = node;
            break;
        }
    }

    if (found) {
        DLL_DEL(&g_ulc_xdp_list, &node->link_node);
    }

    return found;
}

int ULC_HookPoint_XdpAttach(int fd)
{
    ULC_HOOKPOINT_NODE_S *node;
    int ret;

    ret = ULC_FD_IncRef(fd);
    if (ret < 0) {
        return ret;
    }

    node = RcuEngine_ZMalloc(sizeof(ULC_HOOKPOINT_NODE_S));
    if (! node) {
        ULC_FD_DecRef(fd);
        RETURN(BS_NO_MEMORY);
    }

    node->fd = fd;

    MUTEX_P(&g_ulc_hookpoint_mutex);
    DLL_ADD_RCU(&g_ulc_xdp_list, &node->link_node);
    MUTEX_V(&g_ulc_hookpoint_mutex);

    return 0;
}

int ULC_HookPoint_XdpDetach(int fd)
{
    ULC_HOOKPOINT_NODE_S *node;

    MUTEX_P(&g_ulc_hookpoint_mutex);
    node = ulc_hookpoint_xdp_detach(fd);
    MUTEX_V(&g_ulc_hookpoint_mutex);

    if (node) {
        RcuEngine_Free(node);
        ULC_FD_DecRef(fd);
    }

    return 0;
}

int ULC_HookPoint_Init()
{
    EHUB_Reg(EHUB_EV_PRECVER_PKT, "ulc", ulc_hookpoint_precver_pkt_input, NULL);
    return 0;
}

CONSTRUCTOR(init) {
    MUTEX_Init(&g_ulc_hookpoint_mutex);
}
