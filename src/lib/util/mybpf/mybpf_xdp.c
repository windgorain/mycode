/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_bpf.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_hookpoint.h"
#include "utl/ufd_utl.h"
#include "mybpf_def.h"
#include "mybpf_osbase.h"

typedef struct {
    DLL_NODE_S link_node;
    int fd;
}MYBPF_HOOKPOINT_NODE_S;

static inline int _mybpf_process_xdp_fd(MYBPF_RUNTIME_S *runtime, MYBPF_XDP_BUFF_S *xdp_buf, int fd)
{
    UINT64 bpf_ret;
    int ret;

    ret = MYBPF_PROG_Run(runtime, fd, &bpf_ret, (long)xdp_buf, 0, 0, 0, 0);
    if (ret < 0) {
        return XDP_PASS;
    }

    return bpf_ret;
}

static MYBPF_HOOKPOINT_NODE_S * _mybpf_xdp_detach(DLL_HEAD_S *list, int fd)
{
    MYBPF_HOOKPOINT_NODE_S *node;
    MYBPF_HOOKPOINT_NODE_S *found = NULL;

    DLL_SCAN(list, node) {
        if (node->fd == fd) {
            found = node;
            break;
        }
    }

    if (found) {
        DLL_DEL(list, &node->link_node);
    }

    return found;
}

int MYBPF_XdpAttach(MYBPF_RUNTIME_S *runtime, int fd)
{
    MYBPF_HOOKPOINT_NODE_S *node;

    UFD_IncRef(runtime->ufd_ctx, fd);

    node = RcuEngine_ZMalloc(sizeof(MYBPF_HOOKPOINT_NODE_S));
    if (! node) {
        UFD_DecRef(runtime->ufd_ctx, fd);
        RETURN(BS_NO_MEMORY);
    }

    node->fd = fd;

    DLL_ADD_RCU(&runtime->xdp_list, &node->link_node);

    return 0;
}

int MYBPF_XdpDetach(MYBPF_RUNTIME_S *runtime, int fd)
{
    MYBPF_HOOKPOINT_NODE_S *node;

    node = _mybpf_xdp_detach(&runtime->xdp_list, fd);

    if (node) {
        RcuEngine_Free(node);
        UFD_DecRef(runtime->ufd_ctx, fd);
    }

    return 0;
}

int MYBPF_XdpInput(MYBPF_RUNTIME_S *runtime, MYBPF_XDP_BUFF_S *xdp_buf)
{
    MYBPF_HOOKPOINT_NODE_S *node;
    DLL_HEAD_S *list = &runtime->xdp_list;
    int ret = XDP_PASS;

    int state = RcuEngine_Lock();

    DLL_SCAN(list, node) {
        ret = _mybpf_process_xdp_fd(runtime, xdp_buf, node->fd);
        if (ret != XDP_PASS) {
            break;
        }
    }

    RcuEngine_UnLock(state);

    return ret;
}

