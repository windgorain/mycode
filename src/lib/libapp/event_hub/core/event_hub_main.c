/*================================================================
*   Created by LiXingang
*   Description: Event Hub
*
================================================================*/
#include "bs.h"
#include "utl/eth_utl.h"
#include "utl/exec_utl.h"
#include "utl/txt_utl.h"
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/udp_utl.h"
#include "utl/ubpf_utl.h"
#include "utl/http_lib.h"
#include "utl/http_monitor.h"
#include "comp/comp_event_hub.h"

#include "../h/event_hub_func.h"

static DLL_HEAD_S g_ehub_pkt_hooks[EHUB_EV_MAX];
static MUTEX_S g_ehub_mutex;

static EHUB_EV_OB_S * ehub_reg(UINT event, char *name, PF_EHUB_FUNC func, void *ud)
{
    EHUB_EV_OB_S *ob;

    if (event >= EHUB_EV_MAX) {
        BS_DBGASSERT(0);
        return NULL;
    }

    if (strnlen(name, EHUB_OB_NAME_SIZE) >= EHUB_OB_NAME_SIZE) {
        BS_DBGASSERT(0);
        return NULL;
    }

    if (EHUB_Find(event, name)) {
        return NULL;
    }

    ob = RcuEngine_ZMalloc(sizeof(EHUB_EV_OB_S));
    if (! ob) {
        return NULL;
    }

    ob->ud = ud;
    ob->pkt_func = func;
    strlcpy(ob->ob_name, name, sizeof(ob->ob_name));
    ob->enabled = 1;

    MUTEX_P(&g_ehub_mutex);
    DLL_ADD_RCU(&g_ehub_pkt_hooks[event], &ob->link_node);
    MUTEX_V(&g_ehub_mutex);

    return ob;
}

PLUG_API EHUB_EV_OB_S * EHUB_Reg(UINT event, char *name, PF_EHUB_FUNC func, void *ud)
{
    return ehub_reg(event, name, func, ud);
}

PLUG_API void EHUB_UnReg(EHUB_EV_OB_S *ob)
{
    if (! ob) {
        return;
    }

    MUTEX_P(&g_ehub_mutex);
    DLL_DelIfInList(&ob->link_node);
    MUTEX_V(&g_ehub_mutex);

    RcuEngine_Free(ob);
}

PLUG_API void EHUB_SetEnable(EHUB_EV_OB_S *ob, BOOL_T enable)
{
    if (ob) {
        BS_DBGASSERT(0);
        return;
    }
    ob->enabled = enable;
}

PLUG_API EHUB_EV_OB_S * EHUB_Find(UINT event, char *name)
{
    EHUB_EV_OB_S *ob;

    if (event >= EHUB_EV_MAX) {
        BS_DBGASSERT(0);
        return NULL;
    }

    DLL_SCAN(&g_ehub_pkt_hooks[event], ob) {
        if (0 == strcmp(name, ob->ob_name)) {
            return ob;
        }
    }

    return NULL;
}

PLUG_API int EHUB_Publish(UINT event, void *data)
{
    EHUB_EV_OB_S *ob;
    DLL_HEAD_S *list;

    if (event >= EHUB_EV_MAX) {
        BS_DBGASSERT(0);
        return 0;
    }

    list = &g_ehub_pkt_hooks[event];

    int state = RcuEngine_Lock();

    DLL_SCAN(list, ob) {
        if (! ob->enabled) {
            continue;
        }

        ob->pkt_func(ob, data);
    }

    RcuEngine_UnLock(state);

    return 0;
}

PLUG_API int EHUB_CmdShow(UINT argc, char **argv, void *env)
{
    EHUB_EV_OB_S *ob;
    DLL_HEAD_S *head;
    int i;

    EXEC_OutInfo(" event enable name \r\n");
    EXEC_OutInfo(" ------------------------------------------------------\r\n");

    int state = RcuEngine_Lock();

    for (i=0; i<EHUB_EV_MAX; i++) {
        head = &g_ehub_pkt_hooks[i];
        DLL_SCAN(head, ob) {
            EXEC_OutInfo(" %-5u %-6u %s \r\n", i, ob->enabled, ob->ob_name);
        }
    }

    RcuEngine_UnLock(state);

    return 0;
}

int EHUB_Init()
{
    return 0;
}

CONSTRUCTOR(init) {
    int i;

    for (i=0; i<EHUB_EV_MAX; i++) {
        DLL_INIT(&g_ehub_pkt_hooks[i]);
    }

    MUTEX_Init(&g_ehub_mutex);
}

