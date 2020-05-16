/*================================================================
*   Created by LiXingang
*   Description: DPDK Ring
*
================================================================*/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/txt_utl.h"
#include "utl/cff_utl.h"
#include "comp/comp_precver.h"
#include "../h/precver_def.h"
#include "../h/precver_conf.h"

#ifdef USE_DPDK

#include "rte_config.h"
#include "rte_common.h"
#include "rte_memory.h"
#include "rte_launch.h"
#include "rte_eal.h"
#include "rte_per_lcore.h"
#include "rte_lcore.h"
#include "rte_debug.h"
#include "rte_atomic.h"
#include "rte_ring.h"
#include "rte_mbuf.h"
#include "rte_eal.h"
#include "rte_ip.h"
#include "rte_tcp.h"
#include "rte_udp.h"


#define MAX_RX_QUEUE_PER_PORT       128

#define CFG_VALUE_LEN	256


struct precver_dpdk_eal_arg {
	int argc;
	char *argv[32];
};

struct precver_corelist_cfg
{
	char * prefix;
};

static int precver_ring_run();

static char* argv[32] = {NULL};
struct precver_corelist_cfg g_precver_corelist = {0};
struct precver_dpdk_eal_arg g_eal_arg;

static int precver_dpdk_init()
{
	CFF_HANDLE hCff;
	int i = 0;
	int cmd_argc = 0;
	BS_STATUS ret;

	memset(&g_eal_arg, 0, sizeof(struct precver_dpdk_eal_arg));

    hCff = PRecver_Conf_Open("precver_dpdk.ini");
    if (!hCff) {
        return -1;
	}

	ret = CFF_GetPropAsString(hCff, "prog_name", "name", &argv[i]);
	if(BS_OK==ret) {
		g_eal_arg.argv[cmd_argc]=strdup(argv[i]);
		cmd_argc++;
		i++;
	}

	ret = CFF_GetPropAsString(hCff, "m_channel", "mem", &argv[i]);
	if(BS_OK == ret) {
		g_eal_arg.argv[cmd_argc]=strdup("-n");
		g_eal_arg.argv[cmd_argc+1]=strdup(argv[i]);
		cmd_argc+=2;
		i++;
	}

	g_eal_arg.argv[cmd_argc]="--proc-type=secondary";
	cmd_argc++;
	g_eal_arg.argc = cmd_argc;

	CFF_Close(hCff);

	return ret;

}

static int precver_ring_init(int argc, char **argv)
{
    int ret;
    cpu_set_t mask;
    static int inited = 0;

    if (inited) {
        return 0;
    }
    inited = 1;

	ret = precver_dpdk_init();
	if(ret != BS_OK) {
		printf("parse fail\r\n");
		RETURN(BS_BAD_PARA);
	}

	sched_getaffinity(0, sizeof(mask), &mask);
	ret = rte_eal_init(g_eal_arg.argc, g_eal_arg.argv);
    if (ret < 0) {
        RETURN(BS_CAN_NOT_OPEN);
    }
    sched_setaffinity(0, sizeof(mask), &mask);

    RTE_PER_LCORE(_lcore_id) = -1;

    return 0;
}

static int precver_ring_run(PRECVER_RUNNER_S *runner)
{
    PRECVER_PKT_S pkt;
    struct rte_mbuf *m;
    struct timeval linux_base_time;
    unsigned long tick_base;
    unsigned long cur_time;
    void *ring = runner->recver_handle;

    gettimeofday(&linux_base_time, NULL);
    tick_base = TM_GetTick();

    pkt.ts.tv_sec = linux_base_time.tv_sec;

    do {
        cur_time = TM_GetTick();

        if(rte_ring_dequeue(ring, (void**)&m) == 0) {
            unsigned long diff_time = cur_time;
            unsigned long diff_us = diff_time/TM_MS_HZ * 1000;
            unsigned long total_usec = linux_base_time.tv_usec + diff_us;

            if (total_usec >= 1000000) {
                pkt.ts.tv_sec = linux_base_time.tv_sec + 1;
                pkt.ts.tv_usec = total_usec - 1000000;
            } else {
                pkt.ts.tv_usec = total_usec;
            }

            pkt.data = rte_pktmbuf_mtod(m, void *);
            pkt.data_len = rte_pktmbuf_data_len(m);
            PRecver_PktInput(runner, &pkt);
            rte_pktmbuf_free(m);
        }
    }while ((cur_time - tick_base < TM_HZ)); /* 1秒后停止循环 */

    return 0;
}

/* argv[0]==ring_name*/
int PRecver_Ring_Run(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    if (argc == 0) {
        printf("Bad ring param.\r\n");
        RETURN(BS_BAD_PARA);
    }

    if (0 != precver_ring_init(argc, argv)) {
        printf("Ring init failed\r\n");
        RETURN(BS_CAN_NOT_OPEN);
    }

    runner->recver_handle = rte_ring_lookup(argv[0]);
    if (runner->recver_handle == NULL) {
        printf("Can't get ring.\r\n");
        RETURN(BS_CAN_NOT_OPEN);
    }

    while (1) {
        precver_ring_run(runner);
        if (runner->need_stop) {
            break;
        }
    }

    return 0;
}

#endif

