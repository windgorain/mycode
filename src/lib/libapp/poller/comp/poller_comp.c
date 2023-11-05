/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mutex_utl.h"
#include "utl/dll_utl.h"
#include "comp/comp_poller.h"
#include "../h/poller_core.h"

static MUTEX_S g_poller_mutex;

PLUG_API void * PollerComp_Get(char *name)
{
    POLLER_INS_S *ins = POLLER_INS_GetByName(name);
    if (! ins) {
        return NULL;
    }

    ATOM_INC_FETCH(&ins->ref);

    return ins;
}

PLUG_API void PollerComp_Free(void *ins)
{
    POLLER_INS_S *the_ins = ins;
    ATOM_DEC_FETCH(&the_ins->ref);
}

PLUG_API MYPOLL_HANDLE PollerComp_GetMyPoll(void *ins)
{
    POLLER_INS_S *the_ins = ins;
    return the_ins->mypoller;
}


PLUG_API void PollerComp_RegOb(void *ins, OB_S *ob)
{
    POLLER_INS_S *the_ins = ins;

    MUTEX_P(&g_poller_mutex);
    DLL_ADD(&the_ins->ob_list, &ob->link_node);
    MUTEX_V(&g_poller_mutex);
}

PLUG_API void PollerComp_UnRegOb(OB_S *ob)
{
    MUTEX_P(&g_poller_mutex);
    DLL_DelIfInList(&ob->link_node);
    MUTEX_V(&g_poller_mutex);
}

PLUG_API void PollerComp_Trigger(void *ins)
{
    if (ins) {
        POLLER_INS_Trigger(ins);
    }
}

int PollerComp_Init()
{
    MUTEX_Init(&g_poller_mutex);

    return 0;
}

