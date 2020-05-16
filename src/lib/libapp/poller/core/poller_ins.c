/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mypoll_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/atomic_utl.h"
#include "utl/ob_utl.h"
#include "comp/comp_poller.h"
#include "../h/poller_core.h"

#define POLLER_INS_EVENT_QUIT 0x1
#define POLLER_INS_EVENT_TRIGGER  0x2

static POLLER_INS_S g_poller_ins[POLLER_INS_MAX];

static void pollerins_Main(IN USER_HANDLE_S *ud)
{
    POLLER_INS_S *ins = ud->ahUserHandle[0];

    MyPoll_Run(ins->mypoller);

    MyPoll_Destory(ins->mypoller);
    ins->mypoller = NULL;
    ins->tid = THREAD_ID_INVALID;
    ins->name[0] = '\0';
}

static void pollerins_TriggerOb(POLLER_INS_S *ins)
{
    OB_S *ob;
    PF_POLLER_TriggerCB func;

    DLL_SCAN(&ins->ob_list, ob) {
        func = ob->func;
        func();
    }
}

static BS_WALK_RET_E pollerins_PollerEvent(UINT uiEvent, USER_HANDLE_S *ud)
{
    if (uiEvent & POLLER_INS_EVENT_QUIT) {
        return BS_WALK_STOP;
    }

    if (uiEvent & POLLER_INS_EVENT_TRIGGER) {
        pollerins_TriggerOb(ud->ahUserHandle[0]);
    }

    return BS_WALK_CONTINUE;
}

int POLLER_INS_Add(char *ins_name)
{
    int i;
    USER_HANDLE_S ud;
    THREAD_ID tid;
    char tname[32];

    for (i=0; i<POLLER_INS_MAX; i++) {
        if (g_poller_ins[i].name[0] == '\0') {
            strlcpy(g_poller_ins[i].name, ins_name, POLLER_INS_NAME_LEN + 1);
            break;
        }
    }

    if (i >= POLLER_INS_MAX) {
        return -1;
    }

    g_poller_ins[i].mypoller = MyPoll_Create();
    if (! g_poller_ins[i].mypoller) {
        g_poller_ins[i].name[0] = '\0';
        return -1;
    }

    DLL_INIT(&g_poller_ins[i].ob_list);

    ud.ahUserHandle[0] = &g_poller_ins[i];
    MyPoll_SetUserEventProcessor(g_poller_ins[i].mypoller,
            pollerins_PollerEvent, &ud);

    snprintf(tname, sizeof(tname), "poller-%s", ins_name);
    tid = THREAD_Create(tname, NULL, pollerins_Main, &ud);
    if (tid == THREAD_ID_INVALID) {
        MyPoll_Destory(g_poller_ins[i].mypoller);
        g_poller_ins[i].mypoller = NULL;
        g_poller_ins[i].name[0] = '\0';
        return -1;
    }

    g_poller_ins[i].tid = tid;

    return i;
}

int POLLER_INS_Del(int id)
{
    if (g_poller_ins[id].ref > 0) {
        EXEC_OutString("Someone use the poller\r\n");
        RETURN(BS_NO_PERMIT);
    }

    if (g_poller_ins[id].tid) {
        MyPoll_PostUserEvent(g_poller_ins[id].mypoller, POLLER_INS_EVENT_QUIT);
    } else {
        if (g_poller_ins[id].mypoller) {
            MyPoll_Destory(g_poller_ins[id].mypoller);
            g_poller_ins[id].mypoller = NULL;
        }
        g_poller_ins[id].name[0] = '\0';
    }

    return 0;
}

int POLLER_INS_DelByName(char *name)
{
    int id = POLLER_INS_GetByName(name);

    if (id < 0) {
        RETURN(BS_NO_SUCH);
    }

    POLLER_INS_Del(id);

    return 0;
}

char * POLLER_INS_GetName(int id)
{
    return g_poller_ins[id].name;
}

POLLER_INS_S * POLLER_INS_GetPoller(int id)
{
    return &g_poller_ins[id];
}

int POLLER_INS_GetByName(char *name)
{
    int i;

    for (i=0; i<POLLER_INS_MAX; i++) {
        if (0 == strcmp(name, g_poller_ins[i].name)) {
            return i;
        }
    }

    return -1;
}

void POLLER_INS_Trigger(POLLER_INS_S *ins)
{
    MyPoll_PostUserEvent(ins->mypoller, POLLER_INS_EVENT_TRIGGER); 
}

