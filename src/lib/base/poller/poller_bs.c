/*================================================================
*   Created by LiXingang
*   Description: 一个公共的基础poller
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/atomic_once.h"

#define POLLERBS_EVENT_TRIGGER  0x1

typedef struct {
    MUTEX_S lock;
    MYPOLL_HANDLE mypoller;
    DLL_HEAD_S ob_list;
}POLLER_BS_S;

enum {
    POLLER_BS_MODE_THREAD = 0, 
    POLLER_BS_MODE_MAIN        
};


static POLLER_BS_S g_poller_bs;
static int g_poller_bs_mode = POLLER_BS_MODE_THREAD;

static void pollerbs_TriggerOb(POLLER_BS_S *ins)
{
    OB_S *ob;
    PF_POLLERBS_TriggerCB func;

    DLL_SCAN(&ins->ob_list, ob) {
        func = ob->func;
        func();
    }
}

static int pollerbs_PollerEvent(UINT uiEvent, USER_HANDLE_S *ud)
{
    if (uiEvent & POLLERBS_EVENT_TRIGGER) {
        pollerbs_TriggerOb(&g_poller_bs);
    }

    return 0;
}

static void pollerbs_Main(IN USER_HANDLE_S *ud)
{
    POLLER_BS_S *ins = &g_poller_bs;

    while (1) {
        MyPoll_Run(ins->mypoller);
    }
}

static int pollerbs_init_once(void *ud)
{
    MUTEX_Init(&g_poller_bs.lock);

    g_poller_bs.mypoller = MyPoll_Create();
    if (! g_poller_bs.mypoller) {
        return -1;
    }

    DLL_INIT(&g_poller_bs.ob_list);

    MyPoll_SetUserEventProcessor(g_poller_bs.mypoller, pollerbs_PollerEvent, NULL);

    if (g_poller_bs_mode == POLLER_BS_MODE_THREAD) {
        THREAD_ID tid = THREAD_Create("pollerbs", NULL, pollerbs_Main, NULL);
        if (tid == THREAD_ID_INVALID) {
            MyPoll_Destory(g_poller_bs.mypoller);
            g_poller_bs.mypoller = NULL;
            return -1;
        }
    }

    return 0;
}

static inline void pollerbs_init()
{
    static ATOM_ONCE_S once = ATOM_ONCE_INIT_VALUE;
    AtomOnce_WaitDo(&once, pollerbs_init_once, NULL);
    return;
}

void PollerBs_SetMainMode()
{
    g_poller_bs_mode = POLLER_BS_MODE_MAIN;
}

void PollerBs_Run()
{
    if (g_poller_bs_mode != POLLER_BS_MODE_MAIN) {
        BS_DBGASSERT(0);
        return;
    }

    pollerbs_Main(NULL);
}

PLUG_API MYPOLL_HANDLE PollerBS_GetPoller()
{
    pollerbs_init();
    return g_poller_bs.mypoller;
}

PLUG_API void PollerBS_RegOb(OB_S *ob)
{
    MUTEX_P(&g_poller_bs.lock);
    DLL_ADD(&g_poller_bs.ob_list, &ob->link_node);
    MUTEX_V(&g_poller_bs.lock);
}

PLUG_API void PollerBS_UnRegOb(OB_S *ob)
{
    MUTEX_P(&g_poller_bs.lock);
    DLL_DelIfInList(&ob->link_node);
    MUTEX_V(&g_poller_bs.lock);
}

PLUG_API void PollerBS_Trigger()
{
    MyPoll_PostUserEvent(g_poller_bs.mypoller, POLLERBS_EVENT_TRIGGER); 
}

