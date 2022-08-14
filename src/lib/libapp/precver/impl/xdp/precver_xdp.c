/*================================================================
*   Created by LiXingang
*   Description: 从XDP接收报文
*
================================================================*/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/rdtsc_utl.h"
#include "utl/afxdp_utl.h"
#include "utl/getopt2_utl.h"
#include "comp/comp_precver.h"
#include "../../h/precver_def.h"
#include "../../h/precver_ev.h"
#include <linux/if_link.h>
#include <bpf/libbpf.h>
#include <bpf/xsk.h>
#include <bpf/bpf.h>

typedef struct {
    PRECVER_RUNNER_S *runner;
    struct timeval *ts;
}PRECVER_XDP_UD_S;

static int precver_xdp_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static void * precver_xdp_open(int argc, char **argv)
{
    char *ifname = "";
    char *bpf_prog = NULL;
    bool force = false;
    bool skb = false;
    GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'o', 'i', "interface", 's', &ifname, "interface", 0},
        {'o', 'b', "bpfprog", 's', &bpf_prog, "will load bpf program insteading of xdp default program", 0},
        {'o', 's', "skb", 'b', &skb, "skb mode, default mode is drv", 0},
        {'o', 'f', "force", 'b', &force, "force to load bpf program", 0},
        {0}
    };
    AFXDP_PARAM_S xdp_para = {
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
        .xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST,
        .bind_flags = 0
    };

	if (BS_OK != GETOPT2_ParseFromArgv0(argc, argv, opts)) {
        precver_xdp_help(opts);
        return NULL;
    }

    if (GETOPT2_IsOptSetted(opts, 'h', NULL)) {
        precver_xdp_help(opts);
        return 0;
    }

    if (force)
    {
        xdp_para.xdp_flags &= ~XDP_FLAGS_UPDATE_IF_NOEXIST;
    }

    if (skb)
    {
        xdp_para.xdp_flags |= XDP_FLAGS_SKB_MODE;
    }
    else
    {
        xdp_para.xdp_flags |= XDP_FLAGS_DRV_MODE;
    }

    if (bpf_prog)
    {
        xdp_para.libbpf_flags |= XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD;
    }


    AFXDP_HANDLE hXdp = AFXDP_Create(ifname, bpf_prog, &xdp_para);

    return hXdp;
}

static void precver_xdp_recv_pkt(void *xdp_pkt, UINT pkt_len, void *ud)
{
    PRECVER_PKT_S pkt;
    PRECVER_XDP_UD_S *info = ud;

    pkt.data = xdp_pkt;
    pkt.data_len = pkt_len;
    pkt.ts = *info->ts;

    PRecver_Ev_Publish(info->runner, &pkt);

    return;
}

static int precver_xdp_read(PRECVER_RUNNER_S *runner, void *xdp, struct timeval *ts)
{
    PRECVER_XDP_UD_S ud;

    ud.runner = runner;
    ud.ts = ts;

    AFXDP_RecvPkt(xdp, 64, precver_xdp_recv_pkt, &ud);

    if (runner->need_stop) {
        return BS_STOP;
    }

    return 0;
}

static void precver_xdp_run(PRECVER_RUNNER_S *runner, void *xdp)
{
    struct timeval linux_base_time;
    struct timeval ts;
    unsigned long tick_base;
    unsigned long cur_time;

    gettimeofday(&linux_base_time, NULL);
    tick_base = RDTSC_Get();
    ts.tv_sec = linux_base_time.tv_sec;

    do {
        cur_time = RDTSC_Get();

        unsigned long diff_time = cur_time - tick_base;
        unsigned long diff_us = diff_time/RDTSC_US_HZ;
        unsigned long total_usec = linux_base_time.tv_usec + diff_us;

        if (total_usec >= 1000000) {
            ts.tv_sec = linux_base_time.tv_sec + 1;
            ts.tv_usec = total_usec - 1000000;
        } else {
            ts.tv_usec = total_usec;
        }
        precver_xdp_read(runner, xdp, &ts);

    } while ((cur_time - tick_base < RDTSC_HZ)); /* 1秒后停止循环 */

    return;
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    void *xdp = precver_xdp_open(argc, argv);
    if (! xdp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    runner->recver_handle = xdp;

    return 0;
}

PLUG_API int PRecverImpl_Run(PRECVER_RUNNER_S *runner)
{
    if (! runner->recver_handle) {
        BS_DBGASSERT(0);
        RETURN(BS_NOT_INIT);
    }

    precver_xdp_run(runner, runner->recver_handle);

    return 0;
}

