/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"


static COMP_NODE_S g_comp_nodes[COMP_ID_MAX];

PLUG_API int COMP_Reg(UINT comp_id, COMP_NODE_S *node)
{
    if (comp_id >= COMP_ID_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }

    g_comp_nodes[comp_id] = *node;

    return 0;
}

PLUG_API void COMP_Unreg(UINT comp_id)
{
    if (comp_id >= COMP_ID_MAX) {
        return;
    }

    g_comp_nodes[comp_id].ioctl_func = NULL;
}

PLUG_API int COMP_Ioctl(UINT comp_id, int cmd, void *data)
{
    PF_COMP_FUNC func;
    int ret = BS_NO_SUCH;

    if (comp_id >= COMP_ID_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }

    func = g_comp_nodes[comp_id].ioctl_func;
    if (func) {
        ret = func(cmd, data);
    }

    return ret;
}

