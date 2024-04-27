/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-9-21
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_MACTBL

#include "bs.h"

#include "utl/vclock_utl.h"
#include "utl/hash_utl.h"
#include "utl/mac_table.h"

#define _MAC_TBL_HASH_BUCKET_NUM (128)

typedef struct
{
    UINT uiEvent;
    PF_MACTBL_NOTIFY_FUNC pfNotifyFunc;
    USER_HANDLE_S stUserHandle;
}_MAC_TBL_NOTIFY_S;

typedef struct MAC_TBL_STRUCT
{
    HANDLE hHashId;
    UINT uiOldTick;
    UINT uiUserDataSize; 
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
static UINT mactbl_GetHashIndexFromMac(void *pstHashNode)
{
    _MAC_TBL_NODE_S *pstMacTblNode = pstHashNode;
    UINT ulIndex = 0;
    UINT ulTmp;
    UINT i;

    for (i=0; i<6; i+=2) {
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

static int mactbl_WalkEach(IN HANDLE hHashId, IN _MAC_TBL_NODE_S *pstNode, IN _MAC_TBL_WALK_USER_HANDLE_S *pstUserHanlde)
{
    pstUserHanlde->pfFunc
        (pstUserHanlde->hMacTblId,
        &(pstNode->stMacNode),
        pstNode->pUserData,
        pstUserHanlde->pUserHandle);

    return 0;
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

static int mactbl_DestoryAll (HANDLE hHashId, _MAC_TBL_NODE_S *pstNode, _MAC_TBL_S *pstMacTbl)
{
    mactbl_Del(pstMacTbl, pstNode);
    return 0;
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

MACTBL_HANDLE MACTBL_CreateInstance(UINT uiUserDataSize)
{
    _MAC_TBL_S *pstMacTbl;
    VCLOCK_INSTANCE_HANDLE hVclockInstance;
    HANDLE hHashId;

    pstMacTbl = MEM_ZMalloc(sizeof(_MAC_TBL_S));
    if (NULL == pstMacTbl) {
        return 0;
    }

    hHashId = HASH_CreateInstance(NULL, _MAC_TBL_HASH_BUCKET_NUM, mactbl_GetHashIndexFromMac);
    if (0 == hHashId) {
        MEM_Free(pstMacTbl);
        return 0;
    }

    hVclockInstance = VCLOCK_CreateInstance(FALSE);
    if (NULL == hVclockInstance) {
        HASH_DestoryInstance(hHashId);
        MEM_Free(pstMacTbl);
    }

    pstMacTbl->hHashId = hHashId;
    pstMacTbl->hVclockInstance = hVclockInstance;
    pstMacTbl->uiOldTick = 0;
    pstMacTbl->uiUserDataSize = uiUserDataSize;

    return pstMacTbl;
}

void MACTBL_DestoryInstance(MACTBL_HANDLE mactbl)
{
    if (mactbl == NULL) {
        return;
    }

    if (mactbl->hHashId) {
        HASH_Walk(mactbl->hHashId, (PF_HASH_WALK_FUNC)mactbl_DestoryAll, mactbl);
        HASH_DestoryInstance(mactbl->hHashId);
    }

    if (mactbl->hVclockInstance)
    {
        VCLOCK_DeleteInstance(mactbl->hVclockInstance);
    }
    
    MEM_Free(mactbl);
}

void MACTBL_SetOldTick(MACTBL_HANDLE mactbl, UINT old_tick)
{
    mactbl->uiOldTick = old_tick;
}

void MACTBL_SetNotify(MACTBL_HANDLE mactbl, UINT uiEvent,
        PF_MACTBL_NOTIFY_FUNC pfNotifyFunc, USER_HANDLE_S *pstUserHandle)
{
    mactbl->stNotify.uiEvent = uiEvent;
    mactbl->stNotify.pfNotifyFunc = pfNotifyFunc;

    if (pstUserHandle) {
        mactbl->stNotify.stUserHandle = *pstUserHandle;
    }

    return;
}

BS_STATUS MACTBL_Add(MACTBL_HANDLE mactbl, IN MAC_NODE_S *pstMacNode,
        IN void *pUserData, MAC_NODE_MODE_E eMode)
{
    _MAC_TBL_NODE_S *pstMacTblNode;
    VCLOCK_HANDLE hVclock = NULL;
    USER_HANDLE_S stUserHandle;
    BOOL_T bIsNew = FALSE;
    BOOL_T bLearn = FALSE;
    VOID *pUserDataTmp = NULL;

    BS_DBGASSERT(0 != mactbl);

    pstMacTblNode = mactbl_Find(mactbl, pstMacNode);
    if (NULL == pstMacTblNode)
    {
        pstMacTblNode = MEM_ZMalloc(sizeof(_MAC_TBL_NODE_S));
        if (NULL == pstMacTblNode)
        {
            RETURN(BS_NO_MEMORY);
        }
        if ((mactbl->uiUserDataSize != 0) && (pUserData != NULL))
        {
            pUserDataTmp = MEM_Malloc(mactbl->uiUserDataSize);
            if (NULL == pUserDataTmp)
            {
                MEM_Free(pstMacTblNode);
                RETURN(BS_NO_MEMORY);
            }
        }

        pstMacTblNode->pUserData = pUserDataTmp;

        pstMacTblNode->stMacNode = *pstMacNode;
        HASH_Add(mactbl->hHashId, (HASH_NODE_S*)pstMacTblNode);
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
            mactbl->uiUserDataSize,
            &pstMacTblNode->stMacNode,
            pstMacTblNode->pUserData);
    }

    if ((pstMacTblNode->stMacNode.uiFlag & MAC_NODE_FLAG_STATIC) == 0)
    {
        if (pstMacTblNode->hVclock == NULL)
        {
            stUserHandle.ahUserHandle[0] = mactbl;
            stUserHandle.ahUserHandle[1] = pstMacTblNode;
            hVclock = VCLOCK_CreateTimer(mactbl->hVclockInstance, mactbl->uiOldTick, mactbl->uiOldTick, TIMER_FLAG_CYCLE, mactbl_Old, &stUserHandle);
            if (NULL == hVclock)
            {
                mactbl_Free(mactbl, pstMacTblNode);
                RETURN(BS_ERR);
            }
            pstMacTblNode->hVclock = hVclock;
        }
        else
        {
            VCLOCK_Refresh(mactbl->hVclockInstance, pstMacTblNode->hVclock);
        }
    }
    else
    {
        if (pstMacTblNode->hVclock != NULL)
        {
            VCLOCK_DestroyTimer(mactbl->hVclockInstance, pstMacTblNode->hVclock);
            pstMacTblNode->hVclock = NULL;
        }
    }

    if (bIsNew)
    {
        mactbl_Notify(mactbl, MAC_TBL_EVENT_ADD, pstMacTblNode);
    }

    return BS_OK;
}

BS_STATUS MACTBL_Del(MACTBL_HANDLE mactbl, IN MAC_NODE_S *pstMacNode)
{
    _MAC_TBL_NODE_S *pstMacTblNode;
    _MAC_TBL_NODE_S stTblNode;

    BS_DBGASSERT(0 != mactbl);

    stTblNode.stMacNode = *pstMacNode;

    pstMacTblNode = HASH_Find(mactbl->hHashId, (PF_HASH_CMP_FUNC)mactbl_CmpNode, (HASH_NODE_S*)&stTblNode);
    if (NULL != pstMacTblNode)
    {
        mactbl_Del(mactbl, pstMacTblNode);
    }

    return BS_OK;
}

BS_STATUS MACTBL_Find(MACTBL_HANDLE mactbl, INOUT MAC_NODE_S *pstMacNode, OUT VOID *pUserData)
{
    _MAC_TBL_NODE_S *pstHashNode;

    pstHashNode = mactbl_Find(mactbl, pstMacNode);

    if (NULL == pstHashNode)
    {
        RETURN(BS_NO_SUCH);
    }

    mactbl_CopyMacNode(&pstHashNode->stMacNode,
        pstHashNode->pUserData,
        mactbl->uiUserDataSize,
        pstMacNode,
        pUserData);

    return BS_OK;
}

VOID MACTBL_Walk(MACTBL_HANDLE mactbl, IN PF_MACTBL_WALK_FUNC pfFunc, IN VOID *pUserHandle)
{
    _MAC_TBL_WALK_USER_HANDLE_S stUserHandle;

    BS_DBGASSERT(0 != mactbl);

    stUserHandle.hMacTblId = mactbl;
    stUserHandle.pfFunc = pfFunc;
    stUserHandle.pUserHandle = pUserHandle;

    HASH_Walk(mactbl->hHashId, (PF_HASH_WALK_FUNC)mactbl_WalkEach, &stUserHandle);
}


VOID MACTBL_TickStep(MACTBL_HANDLE mactbl)
{
    VCLOCK_Step(mactbl->hVclockInstance);
}

