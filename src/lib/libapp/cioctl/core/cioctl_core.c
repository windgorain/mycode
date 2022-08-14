/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "app/cioctl_pub.h"

static DLL_HEAD_S g_cioctl_ob_list = DLL_HEAD_INIT_VALUE(&g_cioctl_ob_list);
static MUTEX_S g_cioctl_lock;

CONSTRUCTOR(init) {
    MUTEX_Init(&g_cioctl_lock);
}

int CCIOCTL_RegOb(CIOCTL_OB_S *ob)
{
    MUTEX_P(&g_cioctl_lock);
    DLL_ADD_RCU(&g_cioctl_ob_list, &ob->link_node);
    MUTEX_V(&g_cioctl_lock);
    return 0;
}

int CCIOCTL_UnRegOb(CIOCTL_OB_S *ob)
{
    MUTEX_P(&g_cioctl_lock);
    DLL_DEL(&g_cioctl_ob_list, &ob->link_node);
    MUTEX_V(&g_cioctl_lock);
    RcuEngine_Sync();
    return 0;
}

int CIOCTL_ProcessRequest(CIOCTL_REQUEST_S *req, OUT VBUF_S *reply)
{
    CIOCTL_OB_S *ob, *found = NULL;
    int ret;

    int state = RcuEngine_Lock();

    DLL_SCAN(&g_cioctl_ob_list, ob) {
        if (0 == strcmp(ob->name, req->name)) {
            found = ob;
            break;
        }
    }

    ret = found->func(req, reply);

    RcuEngine_UnLock(state);

    return ret;
}


