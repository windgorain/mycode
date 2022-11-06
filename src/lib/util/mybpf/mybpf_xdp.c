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

static DLL_HEAD_S g_mybpf_xdp_list = DLL_HEAD_INIT_VALUE(&g_mybpf_xdp_list);

static inline int _mybpf_process_xdp_fd(MYBPF_XDP_BUFF_S *xdp_buf, int fd)
{
    MYBPF_PROG_NODE_S *prog;

    prog = MYBPF_PROG_GetByFD(fd);
    if (! prog) {
        return XDP_PASS;
    }

//    return MYBPF_RunBpfCode(prog->insn, (long)xdp_buf, 0, 0, 0, 0);

    MYBPF_CTX_S ctx = {0};
    ctx.insts = prog->insn;

    int ret = MYBPF_DefultRun(&ctx, (long)xdp_buf, 0, 0, 0, 0);
    if (ret < 0) {
        return XDP_DROP;
    }

    return ctx.bpf_ret;
}

static MYBPF_HOOKPOINT_NODE_S * _mybpf_xdp_detach(int fd)
{
    MYBPF_HOOKPOINT_NODE_S *node;
    MYBPF_HOOKPOINT_NODE_S *found = NULL;

    DLL_SCAN(&g_mybpf_xdp_list, node) {
        if (node->fd == fd) {
            found = node;
            break;
        }
    }

    if (found) {
        DLL_DEL(&g_mybpf_xdp_list, &node->link_node);
    }

    return found;
}

int MYBPF_XdpAttach(int fd)
{
    MYBPF_HOOKPOINT_NODE_S *node;

    UFD_IncRef(fd);

    node = RcuEngine_ZMalloc(sizeof(MYBPF_HOOKPOINT_NODE_S));
    if (! node) {
        UFD_DecRef(fd);
        RETURN(BS_NO_MEMORY);
    }

    node->fd = fd;

    DLL_ADD_RCU(&g_mybpf_xdp_list, &node->link_node);

    return 0;
}

int MYBPF_XdpDetach(int fd)
{
    MYBPF_HOOKPOINT_NODE_S *node;

    node = _mybpf_xdp_detach(fd);

    if (node) {
        RcuEngine_Free(node);
        UFD_DecRef(fd);
    }

    return 0;
}

int MYBPF_XdpInput(MYBPF_XDP_BUFF_S *xdp_buf)
{
    MYBPF_HOOKPOINT_NODE_S *node;
    int ret = XDP_PASS;

    int state = RcuEngine_Lock();

    DLL_SCAN(&g_mybpf_xdp_list, node) {
        ret = _mybpf_process_xdp_fd(xdp_buf, node->fd);
        if (ret != XDP_PASS) {
            break;
        }
    }

    RcuEngine_UnLock(state);

    return ret;
}

