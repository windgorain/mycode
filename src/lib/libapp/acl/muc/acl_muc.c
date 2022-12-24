/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "comp/comp_muc.h"
#include "comp/comp_acl.h"
#include "../h/acl_muc.h"
#include "../h/acl_app_func.h"

static MUC_EVENT_OB_S g_acl_muc_event_ob;

static int acl_muc_event_create(void *ob, int muc_id)
{
    if (MUC_GetUD(muc_id, MUC_UD_ACL)) {
        return 0;
    }

    ACL_MUC_S *node = MEM_ZMalloc(sizeof(ACL_MUC_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    MUC_SetUD(muc_id, MUC_UD_ACL, node);

    return 0;
}

static void acl_muc_destroy_delay(void *pstRcuNode)
{
    ACL_MUC_S *node = container_of(pstRcuNode, ACL_MUC_S, rcu_node);

    AclAppIP_DestroyMuc(node);

    MEM_Free(node);
}

static int acl_muc_event_destroy(void *ob, int muc_id)
{
    ACL_MUC_S *node = MUC_GetUD(muc_id, MUC_UD_ACL);
    if (! node) {
        return 0;
    }

    RcuEngine_Call(&node->rcu_node, acl_muc_destroy_delay);

    return 0;
}

static int acl_muc_event_notify(void *ob, int muc_id, UINT event)
{
    int ret = 0;

    switch (event) {
        case MUC_EVENT_CREATE:
            ret = acl_muc_event_create(ob, muc_id);
            break;
        case MUC_EVENT_DESTROY:
            ret = acl_muc_event_destroy(ob, muc_id);
            break;
    }

    return ret;
}

int AclMuc_Init()
{
    g_acl_muc_event_ob.pfEventFunc = acl_muc_event_notify;
    MUC_RegEventOB(&g_acl_muc_event_ob);

    acl_muc_event_create(&g_acl_muc_event_ob, 0);

    return 0;
}

ACL_MUC_S * AclMuc_Get(int muc_id)
{
    return MUC_GetUD(muc_id, MUC_UD_ACL);
}

