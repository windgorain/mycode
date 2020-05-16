/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/ws_utl.h"

#include "ws_def.h"
#include "ws_conn.h"
#include "ws_trans.h"
#include "ws_event.h"
#include "ws_trans_mempool.h"

BS_STATUS WS_TransMemPool_Init(IN WS_TRANS_S *pstTrans)
{
    pstTrans->hTransMemPool = MEMPOOL_Create(0);
    if (NULL == pstTrans->hTransMemPool)
    {
        return BS_ERR;
    }

    return BS_OK;
}

VOID WS_TransMemPool_Fini(IN WS_TRANS_S *pstTrans)
{
    if (NULL != pstTrans->hTransMemPool)
    {
        MEMPOOL_Destory(pstTrans->hTransMemPool);
    }
}

VOID * WS_TransMemPool_Alloc(IN WS_TRANS_HANDLE hTrans, IN UINT uiSize)
{
    WS_TRANS_S *pstTrans = hTrans;

    return MEMPOOL_Alloc(pstTrans->hTransMemPool, uiSize);
}

VOID * WS_TransMemPool_ZAlloc(IN WS_TRANS_HANDLE hTrans, IN UINT uiSize)
{
    WS_TRANS_S *pstTrans = hTrans;

    return MEMPOOL_ZAlloc(pstTrans->hTransMemPool, uiSize);
}

VOID WS_TransMemPool_Free(IN WS_TRANS_HANDLE hTrans, IN VOID *pMem)
{
    WS_TRANS_S *pstTrans = hTrans;

    MEMPOOL_Free(pstTrans->hTransMemPool, pMem);
}


