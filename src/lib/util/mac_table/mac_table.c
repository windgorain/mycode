/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-9-21
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_MACTBL

#include "bs.h"

#include "utl/vclock_utl.h"
#include "utl/hash_utl.h"
#include "utl/mac_table.h"

#define _MAC_TBL_HASH_BUCKET_NUM (16*1024)

typedef struct
{
    UINT uiEvent;
    PF_MACTBL_NOTIFY_FUNC pfNotifyFunc;
    USER_HANDLE_S stUserHandle;
}_MAC_TBL_NOTIFY_S;

typedef struct
{
    HANDLE hHashId;
    UINT uiOldTick;
    UINT uiUserDataSize; /* 用户数据大小 */
    VCLOCK_INSTANCE_HANDLE hVclockInstance;
    _MAC_TBL_NOTIFY_S stNotify;
}_MAC_TBL_S;

typedef struct
{
    HASH_NODE_S stHashNode;
    VCLOCK_HANDLE hVclock;
    MAC_NODE_S stMacNode;
    VOID *pUserData;
}_MAC_TBL_NODE_S;

typedef struct
{
    HANDLE hMacTblId;
    PF_MACTBL_WALK_FUNC pfFunc;
    VOID *pUserHandle;
}_MAC_TBL_WALK_USER_HANDLE_S;

static VOID mactbl_Notify(IN _MAC_TBL_S *pstMacTbl, IN UINT uiEvent, IN _MAC_TBL_NODE_S *pstMacNode)
{
    UINT uiNotifyEvent;

    uiNotifyEvent = pstMacTbl->stNotify.uiEvent & uiEvent;
    
    if (uiNotifyEvent)
    {
        pstMacTbl->stNotify.pfNotifyFunc(pstMacTbl, uiNotifyEvent,
            &pstMacNode->stMacNode, pstMacNode->pUserData, &pstMacTbl->stNotify.stUserHandle);
    }
}

static UINT mactbl_GetHashIndexFromMac(IN _MAC_TBL_NODE_S *pstMacTblNode)
{
    UINT ulIndex = 0;
    UINT ulTmp;
    UINT i;

    for (i=0; i<6; i+=2)
    {
        ulTmp = pstMacTblNode->stMacNode.stMac.aucMac[i];

        ulIndex += (ulTmp << 8) + pstMacTblNode->stMacNode.stMac.aucMac[i + 1];
    }

    return ulIndex;
}

static INT mactbl_CmpNode(IN _MAC_TBL_NODE_S *pstNode1, IN _MAC_TBL_NODE_S *pstNode2)
{
    UINT i;

    for (i=0; i<6; i++)
    {
        if (pstNode1->stMacNode.stMac.aucMac[i] == pstNode2->stMacNode.stMac.aucMac[i])
        {
            continue;
        }

        return pstNode1->stMacNode.stMac.aucMac[i] - pstNode2->stMacNode.stMac.aucMac[i];
    }

    return 0;
}

static BS_WALK_RET_E mactbl_WalkEach(IN HANDLE hHashId, IN _MAC_TBL_NODE_S *pstNode, IN _MAC_TBL_WALK_USER_HANDLE_S *pstUserHanlde)
{
    pstUserHanlde->pfFunc
        (pstUserHanlde->hMacTblId,
        &(pstNode->stMacNode),
        pstNode->pUserData,
        pstUserHanlde->pUserHandle);

    return BS_WALK_CONTINUE;
}

static inline VOID mactbl_Free(IN _MAC_TBL_S *pstMacTbl, IN _MAC_TBL_NODE_S *pstMacNode)
{
    HASH_Del(pstMacTbl->hHashId, pstMacNode);
    if (pstMacNode->hVclock != NULL)
    {
        VCLOCK_DestroyTimer(pstMacTbl->hVclockInstance, pstMacNode->hVclock);
    }

    if (NULL != pstMacNode->pUserData)
    {
        MEM_Free(pstMacNode->pUserData);
    }
    
    MEM_Free(pstMacNode);
}

static VOID mactbl_Old(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    _MAC_TBL_S *pstMacTbl = pstUserHandle->ahUserHandle[0];
    _MAC_TBL_NODE_S *pstMacNode = pstUserHandle->ahUserHandle[1];

    mactbl_Notify(pstMacTbl, MAC_TBL_EVENT_OLD, pstMacNode);

    mactbl_Free(pstMacTbl, pstMacNode);
}

static VOID mactbl_Del(IN _MAC_TBL_S *pstMacTbl, IN _MAC_TBL_NODE_S *pstMacNode)
{
    mactbl_Notify(pstMacTbl, MAC_TBL_EVENT_DEL, pstMacNode);

    mactbl_Free(pstMacTbl, pstMacNode);
}

static inline _MAC_TBL_NODE_S * mactbl_Find(IN HANDLE hMacTblId, IN MAC_NODE_S *pstMacNode)
{
    _MAC_TBL_S *pstMacTbl;
    _MAC_TBL_NODE_S stTblNode;

    BS_DBGASSERT(0 != hMacTblId);

    pstMacTbl = (_MAC_TBL_S *)hMacTblId;

    stTblNode.stMacNode = *pstMacNode;

    return (_MAC_TBL_NODE_S *)HASH_Find(pstMacTbl->hHashId, (PF_HASH_CMP_FUNC)mactbl_CmpNode, (HASH_NODE_S*)&stTblNode);
}

HANDLE MACTBL_CreateInstance
(
    IN UINT uiUserDataSize/* 用户数据大小 */,
    IN UINT uiOldTick/* 多少个Tick之后老化 */
)
{
    _MAC_TBL_S *pstMacTbl;
    VCLOCK_INSTANCE_HANDLE hVclockInstance;
    HANDLE hHashId;

    pstMacTbl = MEM_ZMalloc(sizeof(_MAC_TBL_S));
    if (NULL == pstMacTbl)
    {
        return 0;
    }

    hHashId = HASH_CreateInstance(_MAC_TBL_HASH_BUCKET_NUM, (PF_HASH_INDEX_FUNC)mactbl_GetHashIndexFromMac);
    if (0 == hHashId)
    {
        MEM_Free(pstMacTbl);
        return 0;
    }

    hVclockInstance = VCLOCK_CreateInstance(FALSE);
    if (NULL == hVclockInstance)
    {
        HASH_DestoryInstance(hHashId);
        MEM_Free(pstMacTbl);
    }

    pstMacTbl->hHashId = hHashId;
    pstMacTbl->hVclockInstance = hVclockInstance;
    pstMacTbl->uiOldTick = uiOldTick;
    pstMacTbl->uiUserDataSize = uiUserDataSize;

    return pstMacTbl;
}

static BS_WALK_RET_E mactbl_DestoryAll
(
    IN HANDLE hHashId,
    IN _MAC_TBL_NODE_S *pstNode,
    IN _MAC_TBL_S *pstMacTbl
)
{
    mactbl_Del(pstMacTbl, pstNode);

    return BS_WALK_CONTINUE;
}

VOID MACTBL_DestoryInstance(IN HANDLE hMacTblId)
{
    _MAC_TBL_S *pstMacTbl;

    if (hMacTblId == NULL)
    {
        return;
    }

    pstMacTbl = (_MAC_TBL_S *)hMacTblId;

    if (pstMacTbl->hHashId)
    {
        HASH_Walk(pstMacTbl->hHashId, (PF_HASH_WALK_FUNC)mactbl_DestoryAll, pstMacTbl);
        HASH_DestoryInstance(pstMacTbl->hHashId);
    }

    if (pstMacTbl->hVclockInstance)
    {
        VCLOCK_DeleteInstance(pstMacTbl->hVclockInstance);
    }
    
    MEM_Free(pstMacTbl);
}

VOID MACTBL_SetNotify
(
    IN HANDLE hMacTbl,
    IN UINT uiEvent,
    IN PF_MACTBL_NOTIFY_FUNC pfNotifyFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _MAC_TBL_S *pstMacTbl;

    pstMacTbl = (_MAC_TBL_S *)hMacTbl;

    pstMacTbl->stNotify.uiEvent = uiEvent;
    pstMacTbl->stNotify.pfNotifyFunc = pfNotifyFunc;
    if (pstUserHandle)
    {
        pstMacTbl->stNotify.stUserHandle = *pstUserHandle;
    }

    return;
}

static VOID mactbl_CopyMacNode
(
    IN MAC_NODE_S *pstMacNodeSrc,
    IN VOID *pUserDataIn,
    IN UINT uiUserDataLen,
    OUT MAC_NODE_S *pstMacNodeDst,
    OUT VOID *pUserDataOut
)
{
    *pstMacNodeDst = *pstMacNodeSrc;

    if ((pUserDataOut != NULL) && (pUserDataIn != NULL))
    {
        memcpy(pUserDataOut, pUserDataIn, uiUserDataLen);
    }
}

BS_STATUS MACTBL_Add
(
    IN HANDLE hMacTblId,
    IN MAC_NODE_S *pstMacNode,
    IN VOID *pUserData,
    IN MAC_NODE_MODE_E eMode
)
{
    _MAC_TBL_NODE_S *pstMacTblNode;
    _MAC_TBL_S *pstMacTbl;
    VCLOCK_HANDLE hVclock = NULL;
    USER_HANDLE_S stUserHandle;
    BOOL_T bIsNew = FALSE;
    BOOL_T bLearn = FALSE;
    VOID *pUserDataTmp = NULL;

    BS_DBGASSERT(0 != hMacTblId);

    pstMacTbl = (_MAC_TBL_S *)hMacTblId;

    pstMacTblNode = mactbl_Find(hMacTblId, pstMacNode);
    if (NULL == pstMacTblNode)
    {
        pstMacTblNode = MEM_ZMalloc(sizeof(_MAC_TBL_NODE_S));
        if (NULL == pstMacTblNode)
        {
            RETURN(BS_NO_MEMORY);
        }
        if ((pstMacTbl->uiUserDataSize != 0) && (pUserData != NULL))
        {
            pUserDataTmp = MEM_Malloc(pstMacTbl->uiUserDataSize);
            if (NULL == pUserDataTmp)
            {
                MEM_Free(pstMacTblNode);
                RETURN(BS_NO_MEMORY);
            }
        }

        pstMacTblNode->pUserData = pUserDataTmp;

        pstMacTblNode->stMacNode = *pstMacNode;
        HASH_Add(pstMacTbl->hHashId, (HASH_NODE_S*)pstMacTblNode);
        bIsNew = TRUE;
        bLearn = TRUE;
    }
    else
    {
        if (eMode == MAC_MODE_SET)
        {
            bLearn = TRUE;
        }
        else
        {
            /* MAC学习, 需要检查优先级 */
            if (pstMacNode->uiPRI >= pstMacTblNode->stMacNode.uiPRI)
            {
                bLearn = TRUE;
            }
        }
    }

    if (bLearn == TRUE)
    {
        mactbl_CopyMacNode(pstMacNode,
            pUserData,
            pstMacTbl->uiUserDataSize,
            &pstMacTblNode->stMacNode,
            pstMacTblNode->pUserData);
    }

    if ((pstMacTblNode->stMacNode.uiFlag & MAC_NODE_FLAG_STATIC) == 0)
    {
        if (pstMacTblNode->hVclock == NULL)
        {
            stUserHandle.ahUserHandle[0] = pstMacTbl;
            stUserHandle.ahUserHandle[1] = pstMacTblNode;
            hVclock = VCLOCK_CreateTimer(pstMacTbl->hVclockInstance, pstMacTbl->uiOldTick, pstMacTbl->uiOldTick, TIMER_FLAG_CYCLE, mactbl_Old, &stUserHandle);
            if (NULL == hVclock)
            {
                mactbl_Free(pstMacTbl, pstMacTblNode);
                RETURN(BS_ERR);
            }
            pstMacTblNode->hVclock = hVclock;
        }
        else
        {
            VCLOCK_Refresh(pstMacTbl->hVclockInstance, pstMacTblNode->hVclock);
        }
    }
    else
    {
        if (pstMacTblNode->hVclock != NULL)
        {
            VCLOCK_DestroyTimer(pstMacTbl->hVclockInstance, pstMacTblNode->hVclock);
            pstMacTblNode->hVclock = NULL;
        }
    }

    if (bIsNew)
    {
        mactbl_Notify(pstMacTbl, MAC_TBL_EVENT_ADD, pstMacTblNode);
    }

    return BS_OK;
}

BS_STATUS MACTBL_Del(IN HANDLE hMacTblId, IN MAC_NODE_S *pstMacNode)
{
    _MAC_TBL_NODE_S *pstMacTblNode;
    _MAC_TBL_S *pstMacTbl;
    _MAC_TBL_NODE_S stTblNode;

    BS_DBGASSERT(0 != hMacTblId);

    pstMacTbl = (_MAC_TBL_S *)hMacTblId;

    stTblNode.stMacNode = *pstMacNode;

    pstMacTblNode = HASH_Find(pstMacTbl->hHashId, (PF_HASH_CMP_FUNC)mactbl_CmpNode, (HASH_NODE_S*)&stTblNode);
    if (NULL != pstMacTblNode)
    {
        mactbl_Del(pstMacTbl, pstMacTblNode);
    }

    return BS_OK;
}

BS_STATUS MACTBL_Find(IN HANDLE hMacTblId, INOUT MAC_NODE_S *pstMacNode, OUT VOID *pUserData)
{
    _MAC_TBL_NODE_S *pstHashNode;
    _MAC_TBL_S *pstMacTbl;

    pstMacTbl = (_MAC_TBL_S *)hMacTblId;

    pstHashNode = mactbl_Find(hMacTblId, pstMacNode);

    if (NULL == pstHashNode)
    {
        RETURN(BS_NO_SUCH);
    }

    mactbl_CopyMacNode(&pstHashNode->stMacNode,
        pstHashNode->pUserData,
        pstMacTbl->uiUserDataSize,
        pstMacNode,
        pUserData);

    return BS_OK;
}

VOID MACTBL_Walk(IN HANDLE hMacTblId, IN PF_MACTBL_WALK_FUNC pfFunc, IN VOID *pUserHandle)
{
    _MAC_TBL_WALK_USER_HANDLE_S stUserHandle;
    _MAC_TBL_S *pstMacTbl;

    BS_DBGASSERT(0 != hMacTblId);

    pstMacTbl = (_MAC_TBL_S *)hMacTblId;

    stUserHandle.hMacTblId = hMacTblId;
    stUserHandle.pfFunc = pfFunc;
    stUserHandle.pUserHandle = pUserHandle;

    HASH_Walk(pstMacTbl->hHashId, (PF_HASH_WALK_FUNC)mactbl_WalkEach, &stUserHandle);
}

/* 触发一次时钟 */
VOID MACTBL_TickStep(IN HANDLE hMacTblId)
{
    _MAC_TBL_S *pstMacTbl;

    pstMacTbl = (_MAC_TBL_S *)hMacTblId;

    VCLOCK_Step(pstMacTbl->hVclockInstance);
}

