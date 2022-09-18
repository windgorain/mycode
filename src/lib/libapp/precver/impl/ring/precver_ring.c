/*================================================================
*   Created by LiXingang
*   Description: DPDK Ring
*
================================================================*/
#include "bs.h"
#include "utl/rdtsc_utl.h"
#include "utl/time_utl.h"
#include "utl/txt_utl.h"
#include "utl/cff_utl.h"
#include "comp/comp_precver.h"
#include "../../h/precver_def.h"
#include "../../h/precver_conf.h"
#include "../../h/precver_ev.h"
#include "../precver_impl_common.h"

#ifdef USE_DPDK

#include "rte_config.h"
#include "rte_common.h"
#include "rte_memory.h"
#include "rte_launch.h"
#include "rte_eal.h"
#include "rte_per_lcore.h"
#include "rte_lcore.h"
#include "rte_debug.h"
//#include "rte_atomic.h"
#include "rte_ring.h"
#include "rte_mbuf.h"
#include "rte_eal.h"
//#include "rte_ip.h"
#include "rte_tcp.h"
#include "rte_udp.h"


#define MAX_RX_QUEUE_PER_PORT       128

#define CFG_VALUE_LEN    256

struct precver_dpdk_eal_arg {
    int argc;
    char *argv[32];
};

struct precver_corelist_cfg
{
    char * prefix;
};

struct rte_ring * g_precver_ring[RTE_MAX_LCORE];
int g_ring_num = 0;
int g_ring_index = 0;

static int precver_ring_run();

static char* argv[32] = {NULL};
struct precver_corelist_cfg g_precver_corelist = {0};
struct precver_dpdk_eal_arg g_eal_arg;

int precver_ring_checkdelimt(char *string, char delim1, char delim2)
{
    int i;

    for (i=0; i<strlen(string); i++) {
        if (isdigit(string[i])) {
            continue;
        } else if (string[i] == delim1 || string[i] == delim2) {
            continue;
        } else if (string[i] == ' ') {
            continue;
        } else {
            return -1;
        }
    }

    return 0;
}

int precver_num_parselist(const char *bp, int list[], int max_len)
{
    unsigned int a, b;
    int len=0;

        do {
                if (!isdigit(*bp)){
                        bp++;
                        continue;
                }
                b = a = strtoul(bp, (char **)&bp, 10);
                if (*bp == '-') {
                        bp++;
                        if (!isdigit(*bp))
                                return -EINVAL;
                        b = strtoul(bp, (char **)&bp, 10);
                }
                if (!(a <= b))
                        return -ERANGE;
                if (b >= max_len)
                        return -ERANGE;
                while (a <= b) {
                        list[len] = a;
                        len++;
                        a++;
                }

                if (*bp == ',' || *bp == ' ')
                        bp++;
        } while (*bp != '\0' && *bp != '\n');

        return len;
}



int precver_dpdk_parse_ring(char* ring_str, int* index_array)
{
    int ring_num = 0;
    char buffer_noblank[128];
    int i, j;
    int ret;

    if (strlen(ring_str) == 0) {
        return -2;
    }

    for (i=0,j=0; i<strlen(ring_str); i++) {
        if (ring_str[i] != ' ') {
            buffer_noblank[j] = ring_str[i];
            j++;
        }
    }
    buffer_noblank[j] = '\0';

    ret = precver_ring_checkdelimt(buffer_noblank, ',', '-');
    if(ret < 0) {
        return -2;
    }

    ring_num = precver_num_parselist(buffer_noblank, index_array, 128);

    return ring_num;
}



static int precver_dpdk_init(void)
{
    CFF_HANDLE hCff;
    int i = 0;
    int cmd_argc = 0;
    BS_STATUS ret;

    memset(&g_eal_arg, 0, sizeof(struct precver_dpdk_eal_arg));
    memset(g_precver_ring, 0, sizeof(g_precver_ring));

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
    int ring_num;
    int ring_index[RTE_MAX_LCORE];
    char ring_name[128];
    struct rte_ring* ring;
    int i;

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


    memset(ring_index, 0, sizeof(ring_index));
    ring_num = precver_dpdk_parse_ring(argv[0], ring_index);
    if (ring_num <= 0) {
        printf("ring index error \r\n");
        return -1;
    }

    for (i=0; i<ring_num; i++) {
        scnprintf(ring_name, sizeof(ring_name), "precver_ring_%d", ring_index[i]);
        ring = rte_ring_lookup(ring_name);
        if (ring) {
            g_precver_ring[i] = ring;
            printf("get ring %s success \r\n", ring_name);
        } else {
            printf("get ring %s failed \r\n", ring_name);
        }
    }

    g_ring_num = ring_num;

    return 0;
}

static int precver_ring_get_mbuf(struct rte_ring** ring_array, struct rte_mbuf**m)
{
    int index = g_ring_index;
    *m = NULL;

    do {
        if(ring_array[index]) {
            rte_ring_dequeue(ring_array[index], (void**)m);
        }
        if(!(*m)) {
            index++;
            if(index == g_ring_num) {
                index = 0;
            }
            if(index == g_ring_index) {
                break;
            }
        }
    }while(!(*m));

    if(!(*m)) {
        return -1;
    }
    g_ring_index = index;

    return 0;
}

static int precver_ring_run(PRECVER_RUNNER_S *runner)
{
    PRECVER_PKT_S pkt;
    struct rte_mbuf *m = NULL;
    struct timeval linux_base_time;
    unsigned long tick_base;
    unsigned long cur_time;

    gettimeofday(&linux_base_time, NULL);
    tick_base = RDTSC_Get();
    pkt.ts.tv_sec = linux_base_time.tv_sec;

    do {
        cur_time = RDTSC_Get();

        if(precver_ring_get_mbuf(g_precver_ring, &m)==0) {
            unsigned long diff_time = cur_time - tick_base;
            unsigned long diff_us = diff_time/RDTSC_US_HZ;
            unsigned long total_usec = linux_base_time.tv_usec + diff_us;

            if (total_usec >= 1000000) {
                pkt.ts.tv_sec = linux_base_time.tv_sec + 1;
                pkt.ts.tv_usec = total_usec - 1000000;
            } else {
                pkt.ts.tv_usec = total_usec;
            }

            pkt.data = rte_pktmbuf_mtod(m, void *);
            pkt.data_len = rte_pktmbuf_data_len(m);

            PRecver_Ev_Publish(runner, &pkt);

            rte_pktmbuf_free(m);
        }
    }while ((cur_time - tick_base < RDTSC_HZ)); /* 1秒后停止循环 */

    return 0;
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    if (argc == 0) {
        printf("Bad ring param.\r\n");
        RETURN(BS_BAD_PARA);
    }

    if (0 != precver_ring_init(argc, argv)) {
        printf("Ring init failed\r\n");
        RETURN(BS_CAN_NOT_OPEN);
    }

    runner->recver_handle = &g_precver_ring;

    return 0;
}

PLUG_API int PRecverImpl_Run(void *runner)
{
    precver_ring_run(runner);
    return 0;
}

#endif

