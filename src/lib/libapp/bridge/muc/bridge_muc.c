/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "comp/comp_muc.h"

static MUC_EVENT_OB_S g_bridge_muc_event_ob;

static int bridge_muc_event_create(void *ob, int muc_id)
{
    if (MUC_GetUD(muc_id, MUC_UD_BRIDGE)) {
        return 0;
    }

    BRIDGE_MUC_S *node = MEM_ZMalloc(sizeof(BRIDGE_MUC_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    MUC_SetUD(muc_id, MUC_UD_BRIDGE, node);

    return 0;
}

static void bridge_muc_destroy_delay(void *pstRcuNode)
{
    BRIDGE_MUC_S *node = container_of(pstRcuNode, BRIDGE_MUC_S, rcu_node);


    MEM_Free(node);
}

static int bridge_muc_event_destroy(void *ob, int muc_id)
{
    BRIDGE_MUC_S *node = MUC_GetUD(muc_id, MUC_UD_BRIDGE);
    if (! node) {
        return 0;
    }

    RcuEngine_Call(&node->rcu_node, bridge_muc_destroy_delay);

    return 0;
}

static int bridge_muc_event_notify(void *ob, int muc_id, UINT event)
{
    int ret = 0;

    switch (event) {
        case MUC_EVENT_CREATE:
            ret = bridge_muc_event_create(ob, muc_id);
            break;
        case MUC_EVENT_DESTROY:
            ret = bridge_muc_event_destroy(ob, muc_id);
            break;
    }

    return ret;
}

int BridgeMuc_Init()
{
    g_bridge_muc_event_ob.pfEventFunc = bridge_muc_event_notify;
    MUC_RegEventOB(&g_bridge_muc_event_ob);

    bridge_muc_event_create(&g_bridge_muc_event_ob, 0);

    return 0;
}

BRIDGE_MUC_S * BridgeMuc_Get(int muc_id)
{
    return MUC_GetUD(muc_id, MUC_UD_BRIDGE);
}

