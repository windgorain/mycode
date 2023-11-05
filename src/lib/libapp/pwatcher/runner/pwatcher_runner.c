/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/jhash_utl.h"
#include "utl/msgque_utl.h"
#include "utl/event_utl.h"

#include "../pwatcher_pub.h"
#include "../h/pwatcher_def.h"
#include "../h/pwatcher_conf.h"
#include "../h/pwatcher_recver.h"
#include "../h/pwatcher_worker.h"
#include "../h/pwatcher_event.h"
#include "../h/pwatcher_link.h"
#include "../h/pwatcher_timer.h"
#include "../h/pwatcher_rcu.h"
#include "../h/pwatcher_runner.h"

#define PWATCHER_RUNNER_EVENT_PKT   0x1
#define PWATCHER_RUNNER_EVENT_TIMER 0x2

typedef struct {
    EVENT_HANDLE hEvent;
    MSGQUE_HANDLE hMsgQue;
}PWATCHER_RUNNER_S;

static PWATCHER_RUNNER_S g_pwatcher_runner[PWATCHER_WORKER_MAX];
static int g_pwatcher_asyn_runner_count = 0; 

static int pwatcher_runner_event_in(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);

static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .ob_name="runner",
    .event_func=pwatcher_runner_event_in
};

static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_GLOBAL_TIMER},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};

static inline void pwatcher_runner_timer_input(PWATCHER_TIMER_EV_S *ev)
{
    int i;
    for (i=0; i<g_pwatcher_asyn_runner_count; i++) {
        Event_Write(g_pwatcher_runner[i].hEvent, PWATCHER_RUNNER_EVENT_TIMER);
    }
}

static int pwatcher_runner_event_in(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data)
{
    switch (point) {
        case PWATCHER_EV_GLOBAL_TIMER:
            pwatcher_runner_timer_input(data);
            break;
    }

    return 0;
}

static unsigned int pwatcher_runner_pkt_input(void *data, USHORT data_len, struct timeval *ts)
{
    PWATCHER_PKT_DESC_S pkt;
    UCHAR *acl_action;
    BS_ACTION_E pkt_ret = BS_ACTION_UNDEF;
    INT acl_type;

    PWatcherTimer_Step();

    memset(&pkt, 0, sizeof(pkt));
    pkt.recver_pkt.data = data;
    pkt.recver_pkt.data_len = data_len;
    pkt.recver_pkt.ts = *ts;
    pkt.worker_id = PWatcherWorker_SelfID();

    PWatcherLink_Input(&pkt);
    
    if (pkt.session) {
        acl_action = pkt.session->acl_actions;
        for (acl_type=ACL_TYPE_MAX-1; acl_type >=0; acl_type--) {
            if (BS_ACTION_UNDEF != acl_action[acl_type]) {
                pkt_ret = acl_action[acl_type];
                break;
            }
        }
    }

    return pkt_ret;
}

static inline void pwatcher_runner_pkt(MSGQUE_MSG_S *msg)
{
    struct timeval ts;
    void *data = msg->ahMsg[0];
    UINT data_len = HANDLE_UINT(msg->ahMsg[1]);
    ts.tv_sec = HANDLE_UINT(msg->ahMsg[2]);
    ts.tv_usec = HANDLE_UINT(msg->ahMsg[3]);
    pwatcher_runner_pkt_input(data, data_len, &ts);
    MEM_Free(data);
}

static void pwatcher_runner_main(USER_HANDLE_S *ud)
{
    UINT64 event;
    MSGQUE_MSG_S stMsg;
    int index = HANDLE_UINT(ud->ahUserHandle[0]);
    PWATCHER_RUNNER_S *runner = &g_pwatcher_runner[index];

    while (1) {
        Event_Read(runner->hEvent, EVENT_ALL, &event, EVENT_FLAG_WAIT, BS_WAIT_FOREVER);

        if (event & PWATCHER_RUNNER_EVENT_TIMER) {
            PWatcherTimer_Step();
        }

        if (event & PWATCHER_RUNNER_EVENT_PKT) {
            while(0 == MSGQUE_ReadMsg(runner->hMsgQue, &stMsg)) {
                pwatcher_runner_pkt(&stMsg);
            }
        }
    }
}

static int pwatcher_runner_init_que(int index)
{
    g_pwatcher_runner[index].hEvent = Event_Create();
    g_pwatcher_runner[index].hMsgQue = MSGQUE_Create(1024);
    if ((! g_pwatcher_runner[index].hEvent) || (! g_pwatcher_runner[index].hMsgQue)) {
        RETURN(BS_ERR);
    }
    return 0;
}

int PWatcherRunner_Init()
{
    char name[128];
    int i;
    USER_HANDLE_S ud;

    int num = PWatcherConf_GetRunnerNum();
    if (num == 0) {
        return 0;
    }

    if (num > PWATCHER_WORKER_MAX) {
        num = PWATCHER_WORKER_MAX;
    }

    for (i=0; i<num; i++) {
        if (0 != pwatcher_runner_init_que(i)) {
            break;
        }
        ud.ahUserHandle[0] = UINT_HANDLE(i);
        snprintf(name, sizeof(name), "pwatcher-runner%d", i);
        if (THREAD_ID_INVALID == THREAD_Create(name, NULL, pwatcher_runner_main, &ud)) {
            break;
        }
        g_pwatcher_asyn_runner_count ++;
    }

    PWatcherEvent_Reg(g_pwatcher_ob_points);
    g_pwatcher_ob_node.enabled = 1;

    return 0;
}

static void pwatcher_runner_issuer(char *data, UINT data_len, struct timeval *ts)
{
    IP_TUP_KEY_S key;
    MSGQUE_MSG_S msg;
    char *data_tmp;
    UINT index = 0;

    if (data_len <= sizeof(ETH_HEADER_S) + sizeof(IP_HEAD_S)) {
        return;
    }

    data_tmp = MEM_Dup(data, data_len);
    if (! data_tmp) {
        return;
    }

    memset(&key, 0, sizeof(key));

    if (g_pwatcher_asyn_runner_count > 1) {
        IP_HEAD_S *ip_header = (void*)(data + sizeof(ETH_HEADER_S));
        key.ip[0].ip4 = ip_header->unSrcIp.uiIp;
        key.ip[1].ip4 = ip_header->unDstIp.uiIp;
        UINT hash = JHASH_GeneralBuffer(&key, sizeof(key), 0);
        index = hash % g_pwatcher_asyn_runner_count;
    }

    msg.ahMsg[0] = data_tmp;
    msg.ahMsg[1] = UINT_HANDLE(data_len);
    msg.ahMsg[2] = UINT_HANDLE(ts->tv_sec);
    msg.ahMsg[3] = UINT_HANDLE(ts->tv_usec);
    if (0 != MSGQUE_WriteMsg(g_pwatcher_runner[index].hMsgQue, &msg)) {
        MEM_Free(data_tmp);
        return;
    }
    Event_Write(g_pwatcher_runner[index].hEvent, PWATCHER_RUNNER_EVENT_PKT);  
}

unsigned int PWatcherRunner_PktInput(void *data, USHORT data_len, struct timeval *ts)
{
    unsigned int pkt_ret = 0;

    if (g_pwatcher_asyn_runner_count == 0) {
        pkt_ret = pwatcher_runner_pkt_input(data, data_len, ts);
    } else {
        pwatcher_runner_issuer(data, data_len, ts);
    }

    return pkt_ret;
}

void PWatcherRunner_Timer()
{
    if (g_pwatcher_asyn_runner_count == 0) {
        PWatcherTimer_Step();
    } 
}

