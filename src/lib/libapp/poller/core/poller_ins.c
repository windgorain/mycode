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
static POLLER_INS_S g_poller_base;

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
    OB_S *ob, *tmp;
    PF_POLLER_TriggerCB func;

    DLL_SAFE_SCAN(&ins->ob_list, ob, tmp) {
        func = ob->func;
        func(ob);
    }
}

static void pollerins_base_ob()
{
    pollerins_TriggerOb(&g_poller_base);
}

static int pollerins_PollerEvent(UINT uiEvent, USER_HANDLE_S *ud)
{
    if (uiEvent & POLLER_INS_EVENT_QUIT) {
        return BS_STOP;
    }

    if (uiEvent & POLLER_INS_EVENT_TRIGGER) {
        pollerins_TriggerOb(ud->ahUserHandle[0]);
    }

    return 0;
}

POLLER_INS_S * POLLER_INS_Add(char *ins_name)
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
        return NULL;
    }

    g_poller_ins[i].mypoller = MyPoll_Create();
    if (! g_poller_ins[i].mypoller) {
        g_poller_ins[i].name[0] = '\0';
        return NULL;
    }

    DLL_INIT(&g_poller_ins[i].ob_list);

    ud.ahUserHandle[0] = &g_poller_ins[i];
    MyPoll_SetUserEventProcessor(g_poller_ins[i].mypoller,
            pollerins_PollerEvent, &ud);

    scnprintf(tname, sizeof(tname), "poller-%s", ins_name);
    tid = THREAD_Create(tname, NULL, pollerins_Main, &ud);
    if (tid == THREAD_ID_INVALID) {
        MyPoll_Destory(g_poller_ins[i].mypoller);
        g_poller_ins[i].mypoller = NULL;
        g_poller_ins[i].name[0] = '\0';
        return NULL;
    }

    g_poller_ins[i].tid = tid;

    return &g_poller_ins[i];
}

int POLLER_INS_Del(POLLER_INS_S *ins)
{
    if (ins == &g_poller_base) {
        RETURN(BS_NO_PERMIT);
    }

    if (ins->ref > 0) {
        EXEC_OutString("Someone use the poller\r\n");
        RETURN(BS_NO_PERMIT);
    }

    if (ins->tid) {
        MyPoll_PostUserEvent(ins->mypoller, POLLER_INS_EVENT_QUIT);
    } else {
        if (ins->mypoller) {
            MyPoll_Destory(ins->mypoller);
            ins->mypoller = NULL;
        }
        ins->name[0] = '\0';
    }

    return 0;
}

int POLLER_INS_DelByName(char *name)
{
    if (! name) {
        RETURN(BS_NO_SUCH);
    }

    POLLER_INS_S *ins = POLLER_INS_GetByName(name);
    if (!ins) {
        RETURN(BS_NO_SUCH);
    }

    POLLER_INS_Del(ins);

    return 0;
}

char * POLLER_INS_GetName(int id)
{
    return g_poller_ins[id].name;
}

POLLER_INS_S * POLLER_INS_GetByName(char *name)
{
    int i;

    if (NULL == name) {
        return &g_poller_base;
    }

    for (i=0; i<POLLER_INS_MAX; i++) {
        if (0 == strcmp(name, g_poller_ins[i].name)) {
            return &g_poller_ins[i];
        }
    }

    return NULL;
}

void POLLER_INS_Trigger(POLLER_INS_S *ins)
{
    MyPoll_PostUserEvent(ins->mypoller, POLLER_INS_EVENT_TRIGGER); 
}

static OB_S g_poller_base_Ob = {.func = pollerins_base_ob};

void POLLER_INS_Init()
{
    DLL_INIT(&g_poller_base.ob_list);
    g_poller_base.mypoller = PollerBS_GetPoller();
    PollerBS_RegOb(&g_poller_base_Ob);
}

