/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "comp/comp_muc.h"
#include "../h/pwatcher_muc.h"
#include "../h/pwatcher_zone.h"

static MUC_EVENT_OB_S g_pwatcher_muc_event_ob;

static int pwatcher_muc_event_create(void *ob, int muc_id)
{
    if (MUC_GetUD(muc_id, MUC_UD_PWATCHER)) {
        return 0;
    }

    PWATCHER_MUC_S *node = MEM_ZMalloc(sizeof(PWATCHER_MUC_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    MUC_SetUD(muc_id, MUC_UD_PWATCHER, node);

    return 0;
}

static void pwatcher_muc_destroy_delay(void *pstRcuNode)
{
    PWATCHER_MUC_S *node = container_of(pstRcuNode, PWATCHER_MUC_S, rcu_node);

    PWatcherZone_MucDestroy(node);

    MEM_Free(node);
}

static int pwatcher_muc_event_destroy(void *ob, int muc_id)
{
    PWATCHER_MUC_S *node = MUC_GetUD(muc_id, MUC_UD_PWATCHER);
    if (! node) {
        return 0;
    }

    RcuEngine_Call(&node->rcu_node, pwatcher_muc_destroy_delay);

    return 0;
}

static int pwatcher_muc_event_notify(void *ob, int muc_id, UINT event)
{
    int ret = 0;

    switch (event) {
        case MUC_EVENT_CREATE:
            ret = pwatcher_muc_event_create(ob, muc_id);
            break;
        case MUC_EVENT_DESTROY:
            ret = pwatcher_muc_event_destroy(ob, muc_id);
            break;
    }

    return ret;
}

int PWatcherMuc_Init()
{
    g_pwatcher_muc_event_ob.pfEventFunc = pwatcher_muc_event_notify;

    MUC_RegEventOB(&g_pwatcher_muc_event_ob);

    pwatcher_muc_event_create(&g_pwatcher_muc_event_ob, 0);

    return 0;
}

PWATCHER_MUC_S * PWatcherMuc_Get(int muc_id)
{
    return MUC_GetUD(muc_id, MUC_UD_PWATCHER);
}
