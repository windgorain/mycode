/*================================================================
*   Created by LiXingang
*   Description: 从SHM接收报文
*
================================================================*/
#include <sys/shm.h>
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/rdtsc_utl.h"
#include "utl/socket_utl.h"
#include "utl/getopt2_utl.h"
#include "utl/data_que.h"
#include "utl/shm_utl.h"
#include "comp/comp_precver.h"
#include "../../h/precver_def.h"
#include "../../h/precver_ev.h"

static int precver_shm_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static DATA_QUE_S * precver_shm_open(int argc, char **argv)
{
    UINT key;
    GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'o', 'k', "key", 'u', &key, "shm key", 0},
        {0}
    };
    
	if (BS_OK != GETOPT2_ParseFromArgv0(argc, argv, opts)) {
        precver_shm_help(opts);
        return NULL;
    }

    if (0 == GETOPT2_IsOptSetted(opts, 'k', NULL)) {
        precver_shm_help(opts);
        return NULL;
    }

    int shm_id = shmget(key, SHM_SIZE + sizeof(DATA_QUE_S), 0644);
    if (shm_id < 0) {
        printf("Get shm failed \r\n");
        return NULL;
    }

    DATA_QUE_S *data_que = shmat(shm_id, 0, 0);
    if ((data_que == NULL) || ((int)(UINT)(ULONG)data_que == -1)) {
        printf("Shmat failed \r\n");
        return NULL;
    }

    DataQue_InitReader(data_que, data_que + 1);

    return data_que;
}

static int precver_shm_read(PRECVER_RUNNER_S *runner, DATA_QUE_S *que, struct timeval *ts)
{
    PRECVER_PKT_S pkt;
    DATA_QUE_DATA_S *que_data;

    que_data = DataQue_Get(que);
    if (NULL == que_data) {
        Sleep(10);
        return 0;
    }

    pkt.data = que_data->data;
    pkt.data_len = que_data->len;
    pkt.ts = *ts;

    PRecver_Ev_Publish(runner, &pkt);

    DataQue_Pop(que, que_data);

    if (runner->need_stop) {
        return BS_STOP;
    }

    return 0;
}

static void precver_shm_run(PRECVER_RUNNER_S *runner, DATA_QUE_S *que)
{
    struct timeval linux_base_time;
    struct timeval ts;
    unsigned long tick_base;
    unsigned long cur_time;

    if (que == NULL) {
        return;
    }

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
        precver_shm_read(runner, que, &ts);

    } while ((cur_time - tick_base < RDTSC_HZ)); /* 1秒后停止循环 */

    return;
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    void *que = precver_shm_open(argc, argv);
    if (que == NULL) {
        RETURN(BS_CAN_NOT_OPEN);
    }
    runner->recver_handle = que;

    return 0;
}

PLUG_API int PRecverImpl_Run(PRECVER_RUNNER_S *runner)
{
    precver_shm_run(runner, runner->recver_handle);
    return 0;
}

