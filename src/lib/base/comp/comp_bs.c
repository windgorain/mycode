/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-8
* Description: 组件
* History:     
******************************************************************************/
#include "bs.h"

static DLL_HEAD_S g_stCompList = DLL_HEAD_INIT_VALUE(&g_stCompList);
static MUTEX_S g_comp_lock;

CONSTRUCTOR(init) {
    MUTEX_Init(&g_comp_lock);
}

PLUG_API void COMP_Reg(COMP_NODE_S *node)
{
    node->ref = 0;

    ATOM_BARRIER();

    MUTEX_P(&g_comp_lock);
    DLL_ADD(&g_stCompList, &node->link_node);
    MUTEX_V(&g_comp_lock);
}

PLUG_API int COMP_UnReg(COMP_NODE_S *node)
{
    if (! DLL_IN_LIST(&node->link_node)) {
        return 0;
    }

    int busy = 1;

    MUTEX_P(&g_comp_lock);
    if (node->ref == 0) {
        DLL_DEL(&g_stCompList, &node->link_node);
        busy = 0;
    }
    MUTEX_V(&g_comp_lock);

    if (busy) {
        RETURN(BS_BUSY);
    }

    return 0;
}

PLUG_API VOID * COMP_Ref(CHAR *comp_name)
{
    COMP_NODE_S *pstCompNode;

    MUTEX_P(&g_comp_lock);
    DLL_SCAN(&g_stCompList, pstCompNode) {
        if (strcmp(pstCompNode->comp_name, comp_name) == 0) {
            pstCompNode->ref ++;
            break;
        }
    }
    MUTEX_V(&g_comp_lock);

    if (NULL == pstCompNode) {
        printf("Can't find comp %s.\r\n", comp_name);
    }

    return pstCompNode;
}

PLUG_API void COMP_Deref(COMP_NODE_S *node)
{
    MUTEX_P(&g_comp_lock);
    node->ref --;
    MUTEX_V(&g_comp_lock);
}
