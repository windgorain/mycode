/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-6-8
* Description: 对申请的内存放到一个pool中进行管理, 并在销毁pool时进行统一释放
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/mutex_utl.h"
#include "utl/mempool_utl.h"

typedef struct
{
    DLL_HEAD_S stMemList;
    UINT uiFlag;
    MUTEX_S stMutex;
}_MEM_POOL_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
}_MEM_POOL_NODE_S;

static inline void mempool_Lock(_MEM_POOL_S *pstPool)
{
    if (pstPool->uiFlag & MEMPOOL_FLAG_WITH_LOCK)
    {
        MUTEX_P(&pstPool->stMutex);
    }
}

static inline void mempool_UnLock(_MEM_POOL_S *pstPool)
{
    if (pstPool->uiFlag & MEMPOOL_FLAG_WITH_LOCK)
    {
        MUTEX_V(&pstPool->stMutex);
    }
}

MEMPOOL_HANDLE MEMPOOL_Create(IN UINT uiFlag)
{
    _MEM_POOL_S *pstPool;

    pstPool = MEM_ZMalloc(sizeof(_MEM_POOL_S));
    if (NULL == pstPool)
    {
        return NULL;
    }

    DLL_INIT(&pstPool->stMemList);

    if (uiFlag & MEMPOOL_FLAG_WITH_LOCK)
    {
        MUTEX_Init(&pstPool->stMutex);
    }

    pstPool->uiFlag = uiFlag;

    return pstPool;
}

VOID * MEMPOOL_Alloc(IN MEMPOOL_HANDLE hMemPool, IN UINT uiSize)
{
    _MEM_POOL_S *pstPool;
    _MEM_POOL_NODE_S *pstNode;
    VOID *pMem;

    if (NULL == hMemPool)
    {
        return MEM_Malloc(uiSize);
    }

    pstPool = hMemPool;
    
    pstNode = MEM_Malloc(uiSize + sizeof(_MEM_POOL_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    mempool_Lock(pstPool);
    DLL_ADD(&pstPool->stMemList, pstNode);
    mempool_UnLock(pstPool);

    pMem = (VOID*)(pstNode + 1);

    return pMem;
}

VOID MEMPOOL_Free(IN MEMPOOL_HANDLE hMemPool, IN VOID *pMem)
{
    _MEM_POOL_S *pstPool;
    _MEM_POOL_NODE_S *pstNode;

    if (NULL == hMemPool)
    {
        MEM_Free(pMem);
        return;
    }

    pstPool = hMemPool;

    pstNode = (_MEM_POOL_NODE_S*)((UCHAR*)pMem - sizeof(_MEM_POOL_NODE_S));

    mempool_Lock(pstPool);
    DLL_DEL(&pstPool->stMemList, pstNode);
    mempool_UnLock(pstPool);

    MEM_Free(pstNode);
}

VOID MEMPOOL_Destory(IN MEMPOOL_HANDLE hMemPool)
{
    _MEM_POOL_S *pstPool;
    _MEM_POOL_NODE_S *pstNode, *pstNodeTmp;

    if (NULL == hMemPool)
    {
        return;
    }

    pstPool = hMemPool;

    DLL_SAFE_SCAN(&pstPool->stMemList, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstPool->stMemList, pstNode);
        MEM_Free(pstNode);
    }

    if (pstPool->uiFlag & MEMPOOL_FLAG_WITH_LOCK)
    {
        MUTEX_Final(&pstPool->stMutex);
    }

    MEM_Free(pstPool);
}

CHAR * MEMPOOL_Strdup(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcStr)
{
    UINT uiLen;
    CHAR *pcDup;

    if (NULL == pcStr)
    {
        return NULL;
    }

    uiLen = strlen(pcStr);
    pcDup = MEMPOOL_Alloc(hMemPool, uiLen + 1);
    if (NULL == pcDup)
    {
        return NULL;
    }
    TXT_Strlcpy(pcDup, pcStr, uiLen + 1);

    return pcDup;
}


