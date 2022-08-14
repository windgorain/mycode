/*================================================================
*   Created by LiXingang
*   Description: 从Netmap接收报文
*
================================================================*/
#include "bs.h"
#include <net/netmap.h>
#include <net/netmap_user.h>
#include "utl/time_utl.h"
#include "utl/rdtsc_utl.h"
#include "utl/socket_utl.h"
#include "utl/getopt2_utl.h"
#include "comp/comp_precver.h"
#include "../../h/precver_def.h"
#include "../precver_impl_common.h"

static int precver_netmap_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static int precver_netmap_pkt(PRECVER_RUNNER_S *runner, void *data, int len, struct timeval *ts)
{
    PRECVER_PKT_S pkt;

    pkt.data = data;
    pkt.data_len = len;
    pkt.ts = *ts;

    runner->recv_pkt(runner, &pkt);
    if (runner->need_stop) {
        return BS_STOP;
    }

    return 0;
}

static void precver_netmap_run(PRECVER_RUNNER_S *runner)
{
    struct netmap_if *nifp = runner->recver_handle;
    struct netmap_ring *ring = NETMAP_RX_RING(nifp, 0);
    struct pollfd x[1];
    struct timeval ts;

    x[0].fd = fd;
    x[0].events = POLLIN;
    poll(x, 1, 100);

    gettimeofday(&ts, NULL);

    for (; ring->avail > 0 ; ring->avail--) {
        i = ring->cur;
        buf = NETMAP_BUF(ring, i);
        precver_netmap_pkt(runner, buf, ring->slot[i].len, &ts);
        ring->cur = NETMAP_NEXT(ring, i);
    }

    return 0;
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    static UINT port;
    static char *interface = NULL;
    static GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'o', 'i', "interface", 's', &interface, "netmap device", 0},
        {0}
    };
    struct netmap_if *nifp;
    struct nmreq req;
    int i, len;
    char *buf;

	if (BS_OK != GETOPT2_ParseFromArgv0(argc, argv, opts)) {
        precver_netmap_help(opts);
        return -1;
    }

    if (interface == NULL) {
        fprintf(stderr, "Can't get netmap device \r\n");
        RETURN(BS_ERR);
    }

    int fd = open("/dev/netmap", 0);  // 打开字符设备
    strcpy(req.nr_name, interface);
    ioctl(fd, NIOCREG, &req);  // 注册网卡
    mem = mmap(NULL, req.nr_memsize, PROT_READ|PROT_WRITE, 0, fd, 0);
    nifp = NETMAP_IF(mem, req.nr_offset)

    runner->recver_handle = nifp;

    return fd;
}

PLUG_API int PRecverImpl_Run(PRECVER_RUNNER_S *runner)
{
    precver_netmap_run(runner);
    return 0;
}

