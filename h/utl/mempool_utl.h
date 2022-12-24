/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-6-8
* Description: 
* History:     
******************************************************************************/

#ifndef __MEMPOOL_UTL_H_
#define __MEMPOOL_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define MEMPOOL_FLAG_WITH_LOCK 0x1 /* 内置锁,用于并发申请/释放内存的情况 */


typedef HANDLE MEMPOOL_HANDLE;

MEMPOOL_HANDLE MEMPOOL_Create(IN UINT uiFlag);

VOID * MEMPOOL_Alloc(IN MEMPOOL_HANDLE hMemPool, IN UINT uiSize);

VOID MEMPOOL_Free(IN MEMPOOL_HANDLE hMemPool, IN VOID *pMem);

VOID MEMPOOL_Destory(IN MEMPOOL_HANDLE hMemPool);

static inline VOID * MEMPOOL_ZAlloc(IN MEMPOOL_HANDLE hMemPool, IN UINT uiSize)
{
    VOID *pMem;

    pMem = MEMPOOL_Alloc(hMemPool, uiSize);
    if (NULL != pMem)
    {
        Mem_Zero(pMem, uiSize);
    }

    return pMem;
}

CHAR * MEMPOOL_Strdup(IN MEMPOOL_HANDLE hMemPool, IN CHAR *pcStr);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__MEMPOOL_UTL_H_*/


