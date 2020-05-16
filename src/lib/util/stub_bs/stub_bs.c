/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-8-27
* Description: 用来对BS类函数进行封装成utl
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/rcu_utl.h"

VOID * mem_Malloc(IN UINT uiSize, IN CHAR *pcFileName, IN UINT uiLine)
{
#ifdef SUPPORT_MEM_MANAGED
    return PrivateMEMX_MallocMem(uiSize, pcFileName, uiLine);
#else
    return malloc(uiSize);
#endif
}

VOID mem_Free(IN VOID *pMem, IN CHAR *pcFileName, IN UINT uiLine)
{
#ifdef SUPPORT_MEM_MANAGED
    PrivateMEMX_FreeMem(pMem, pcFileName, uiLine);
#else
    free(pMem);
#endif
}

static RCU_S g_stub_bs_rcu;
static int g_stub_bs_rcu_inited = 0;

static inline void stubrcubs_init()
{
    if (g_stub_bs_rcu_inited == 0) {
        g_stub_bs_rcu_inited = 1;
        RCU_Init(&g_stub_bs_rcu);
    }
}

VOID StubRcuBs_Free(IN RCU_NODE_S *pstRcuNode, IN PF_RCU_FREE_FUNC pfFreeFunc)
{
    stubrcubs_init();
    RCU_Call(&g_stub_bs_rcu, pstRcuNode, pfFreeFunc);
}

UINT StubRcuBs_Lock()
{
    stubrcubs_init();
    return RCU_Lock(&g_stub_bs_rcu);
}

VOID StubRcuBs_UnLock(IN UINT uiPhase)
{
    stubrcubs_init();
    RCU_UnLock(&g_stub_bs_rcu, uiPhase);
}

