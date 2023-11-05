/*================================================================
*   Created by LiXingang
*   Description: 从UDP接收报文
*
================================================================*/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/rdtsc_utl.h"
#include "utl/socket_utl.h"
#include "utl/getopt2_utl.h"
#include "comp/comp_precver.h"
#include "../../h/precver_def.h"
#include "../../h/precver_ev.h"

static int precver_udp_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static int precver_udp_open(int argc, char **argv)
{
    UINT port;
    GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", GETOPT2_V_NONE, NULL, NULL, 0},
        {'o', 'p', "port", GETOPT2_V_U32, &port, "udp service port", 0},
        {0}
    };
    
	if (BS_OK != GETOPT2_ParseFromArgv0(argc, argv, opts)) {
        precver_udp_help(opts);
        return -1;
    }

    if (port == 0) {
        port = 8888;
    }

    int fd = Socket_OpenUdp(0, htons(port));

    return fd;
}

static int precver_udp_read(PRECVER_RUNNER_S *runner, int fd, struct timeval *ts)
{
    unsigned char buffer[2048];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int nrecv;
    PRECVER_PKT_S pkt;

    nrecv = recvfrom(fd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*)&addr, &addr_len);
    if (nrecv <= 0) {
        return BS_AGAIN;
    }

    pkt.data = (void*)buffer;
    pkt.data_len = nrecv;
    pkt.ts = *ts;

    PRecver_Ev_Publish(runner, &pkt);
    if (runner->need_stop) {
        return BS_STOP;
    }

    return 0;
}

static void precver_udp_run(PRECVER_RUNNER_S *runner, int fd)
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
        precver_udp_read(runner, fd, &ts);

    } while ((cur_time - tick_base < RDTSC_HZ)); 

    return;
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    int fd = precver_udp_open(argc, argv);
    if (fd < 0) {
        RETURN(BS_CAN_NOT_OPEN);
    }
    runner->recver_handle = UINT_HANDLE(fd);

    return 0;
}

PLUG_API int PRecverImpl_Run(PRECVER_RUNNER_S *runner)
{
    precver_udp_run(runner, HANDLE_UINT(runner->recver_handle));
    return 0;
}

