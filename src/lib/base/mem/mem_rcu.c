/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-11-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "utl/num_utl.h"
#include "utl/txt_utl.h"
#include "utl/atomic_once.h"
#include "utl/mem_utl.h"
#include "utl/list_dtq.h"
#include "mem_bs.h"
#include "mem_bs_func.h"

VOID * MEM_RcuMalloc(IN UINT uiSize)
{
    UINT uiNewSize;
    UCHAR *pucMem;

    uiNewSize = sizeof(RCU_NODE_S) + uiSize;

    pucMem = MEM_Malloc(uiNewSize);
    if (NULL == pucMem)
    {
        return NULL;
    }

    return pucMem + sizeof(RCU_NODE_S);
}

static void _mem_RcuFreeCallBack(void *pstRcuNode)
{
    MEM_Free(pstRcuNode);
}

VOID MEM_RcuFree(IN VOID *pMem)
{
    UCHAR *pucMem = pMem;

    pucMem -= sizeof(RCU_NODE_S);

    RcuEngine_Call((RCU_NODE_S*)pucMem, _mem_RcuFreeCallBack);
}

