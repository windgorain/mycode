/******************************************************************************
* Copyright (C), LiXingang
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/
#include "ndis.h"

#include "vndis_def.h"    
#include "vndis_pub.h"
#include "vndis_que.h"
#include "vndis_dev.h"
#include "vndis_adapter.h"
#include "vndis_instance.h"
#include "vndis_mem.h"

VOID * VNDIS_MEM_Malloc(IN UINT uiLen)
{
    NDIS_STATUS enStatus;
    VOID *pMem;

    enStatus = NdisAllocateMemoryWithTag(&pMem, uiLen, ((ULONG)'VNDS'));
    if(enStatus != NDIS_STATUS_SUCCESS)
    {
        pMem = NULL;
    }

    return pMem;
}

VOID VNDIS_MEM_Free(IN VOID *pMem, IN UINT uiLen)
{
    NdisFreeMemory(pMem, uiLen, 0);
}

VOID VNDIS_MEM_Copy(IN VOID *pDst, IN VOID *pSrc, IN UINT uiLen)
{
    NdisMoveMemory(pDst, pSrc, uiLen);
}

