/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-8-29
* Description: 
* History:     
******************************************************************************/

/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_OBJECT
    
#include "bs.h"

#include "utl/darray_utl.h"
#include "utl/kv_utl.h"
#include "utl/nap_utl.h"
#include "utl/object_utl.h"

/* 对象的内部属性 */
typedef struct
{
    DARRAY_HANDLE hPropertys;
    HANDLE hKv;
}OBJECT_S;

static inline OBJECT_S * _object_User2Inner(IN VOID *pUserObj)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = (OBJECT_S*)(VOID*)((UCHAR *)pUserObj - sizeof(OBJECT_S));

    return pstInnerObj;
}

static inline VOID * _object_Inner2User(IN OBJECT_S *pstInnerObj)
{
    return pstInnerObj + 1;
}

/* 返回Object集合ID . 失败则返回0 */
HANDLE OBJECT_CreateAggregate
(
    IN UINT uiMaxNum/* 0表示不限制 */,
    IN UINT uiObjSize/* 对象大小 */,
    IN UINT uiFlag
)
{
    HANDLE hNap;
    UINT uiNapFlag = 0;

    if (uiFlag & OBJECT_FLAG_ENABLE_RCU)
    {
        uiNapFlag = NAP_FLAG_RCU;
    }

    hNap = NAP_Create(NAP_TYPE_HASH, uiMaxNum, sizeof(OBJECT_S) + uiObjSize, uiNapFlag);
    
    if (hNap == NULL)
    {
        return NULL;
    }
    
    return hNap;
}

VOID OBJECT_DestroyAggregate(IN HANDLE hObjectAggregate)
{
    if (hObjectAggregate == 0)
    {
        return;
    }

    OBJECT_FreeAllObjects(hObjectAggregate);

    NAP_Destory(hObjectAggregate);

    return;
}

VOID OBJECT_EnableSeq(IN HANDLE hObjectAggregate, IN UINT64 ulMask, IN UINT uiCount)
{
    NAP_EnableSeq(hObjectAggregate, ulMask, 1);
}

VOID * OBJECT_NewObject(IN HANDLE hObjectAggregate)
{
    OBJECT_S *pstInnerObj;

    if (hObjectAggregate == 0)
    {
        return 0;
    }

    pstInnerObj = NAP_ZAlloc(hObjectAggregate);
    if (pstInnerObj == NULL)
    {
        return 0;
    }

    return _object_Inner2User(pstInnerObj);
}

VOID OBJECT_FreeObject(IN HANDLE hObjectAggregate, IN VOID *pUserObj)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(pUserObj);

    if (NULL != pstInnerObj->hPropertys)
    {
        DARRAY_Destory(pstInnerObj->hPropertys);
        pstInnerObj->hPropertys = NULL;
    }

    if (NULL != pstInnerObj->hKv)
    {
        KV_Destory(pstInnerObj->hKv);
        pstInnerObj->hKv = NULL;
    }

    NAP_Free(hObjectAggregate, pstInnerObj);

    return;
}

VOID OBJECT_FreeObjectByID(IN HANDLE hObjectAggregate, IN UINT64 ulID)
{
    VOID *pUserObj;

    pUserObj = OBJECT_GetObjectByID(hObjectAggregate, ulID);
    if (NULL == pUserObj)
    {
        return;
    }

    OBJECT_FreeObject(hObjectAggregate, pUserObj);
}

VOID * OBJECT_GetObjectByID(IN HANDLE hObjectAggregate, IN UINT64 ulID)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hObjectAggregate, ulID);
    if (NULL == pstInnerObj)
    {
        return NULL;
    }

    return _object_Inner2User(pstInnerObj);
}

UINT64 OBJECT_GetIDByObject(IN HANDLE hObjectAggregate, IN VOID *pUserObj)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(pUserObj);

    return NAP_GetIDByNode(hObjectAggregate, pstInnerObj);
}

VOID OBJECT_FreeAllObjects(IN HANDLE hObjectAggregate)
{
    UINT64 ulNodeId = 0;

    while ((ulNodeId = NAP_GetNextID(hObjectAggregate, ulNodeId) != 0))
    {
        OBJECT_FreeObjectByID(hObjectAggregate, ulNodeId);
    }

    return;
}

BS_STATUS OBJECT_SetProperty(IN VOID *pUserObj, IN UINT uiPropertyIndex, IN HANDLE hValue)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(pUserObj);

    if (NULL == pstInnerObj->hPropertys)
    {
        pstInnerObj->hPropertys = DARRAY_Create(0);
        if (NULL == pstInnerObj->hPropertys)
        {
            RETURN(BS_NO_MEMORY);
        }
    }

    return DARRAY_Set(pstInnerObj->hPropertys, uiPropertyIndex, hValue);
}


BS_STATUS OBJECT_SetPropertyByID
(
    IN HANDLE hObjectAggregate,
    IN UINT64 ulObjectId,
    IN UINT uiPropertyIndex,
    IN HANDLE hValue
)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hObjectAggregate, ulObjectId);
    if (NULL == pstInnerObj)
    {
        RETURN(BS_NO_SUCH);
    }

    return OBJECT_SetProperty(_object_Inner2User(pstInnerObj), uiPropertyIndex, hValue);
}

BS_STATUS OBJECT_GetProperty(IN VOID *pUserObj, IN UINT uiPropertyIndex, OUT HANDLE *phValue)
{
    OBJECT_S *pstInnerObj;

    *phValue = NULL;
    
    pstInnerObj = _object_User2Inner(pUserObj);

    if (NULL == pstInnerObj->hPropertys)
    {
        return BS_NO_SUCH;
    }

    if (uiPropertyIndex >= DARRAY_GetSize(pstInnerObj->hPropertys))
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    *phValue = DARRAY_Get(pstInnerObj->hPropertys, uiPropertyIndex);

    return BS_OK;
}

BS_STATUS OBJECT_GetPropertyByID
(
    IN HANDLE hObjectAggregate,
    IN UINT64 ulObjectId,
    IN UINT uiPropertyIndex,
    OUT HANDLE *phValue
)
{
    OBJECT_S *pstInnerObj;

    *phValue = NULL;
    
    pstInnerObj = NAP_GetNodeByID(hObjectAggregate, ulObjectId);
    if (NULL == pstInnerObj)
    {
        RETURN(BS_NO_SUCH);
    }

    return OBJECT_GetProperty(_object_Inner2User(pstInnerObj), uiPropertyIndex, phValue);
}

BS_STATUS OBJECT_SetKeyValue
(
    IN HANDLE hObjectAggregate,
    IN VOID *pUserObj,
    IN CHAR *pcKey,
    IN CHAR *pcValue
)
{
    OBJECT_S *pstInnerObj;
    UINT uiKvFlag = 0;

    pstInnerObj = _object_User2Inner(pUserObj);

    if (NULL == pstInnerObj->hKv)
    {
        if (NAP_GetFlag(hObjectAggregate) & NAP_FLAG_RCU)
        {
            uiKvFlag = KV_FLAG_ENABLE_RCU;
        }

        pstInnerObj->hKv = KV_Create(uiKvFlag);
        if (NULL == pstInnerObj->hKv)
        {
            RETURN(BS_NO_MEMORY);
        }
    }

    return KV_SetKeyValue(pstInnerObj->hKv, pcKey, pcValue);
}

BS_STATUS OBJECT_SetKeyValueByID
(
    IN HANDLE hObjectAggregate,
    IN UINT64 ulObjectId,
    IN CHAR *pcKey,
    IN CHAR *pcValue
)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hObjectAggregate, ulObjectId);
    if (NULL == pstInnerObj)
    {
        RETURN(BS_NO_SUCH);
    }

    return OBJECT_SetKeyValue(hObjectAggregate, _object_Inner2User(pstInnerObj), pcKey, pcValue);
}

CHAR * OBJECT_GetKeyValue(IN VOID *pUserObj, IN CHAR *pcKey)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(pUserObj);

    if (NULL == pstInnerObj->hKv)
    {
        return NULL;
    }

    return KV_GetKeyValue(pstInnerObj->hKv, pcKey);
}

CHAR * OBJECT_GetKeyValueByID
(
    IN HANDLE hObjectAggregate,
    IN UINT64 ulObjectId,
    IN CHAR *pcKey
)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hObjectAggregate, ulObjectId);
    if (NULL == pstInnerObj)
    {
        return NULL;
    }

    return OBJECT_GetKeyValue(_object_Inner2User(pstInnerObj), pcKey);
}

VOID OBJECT_Walk(IN HANDLE hObjectAggregate, IN OBJECT_WALK_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 ulID = 0;
    BS_WALK_RET_E eRet;

    while ((ulID = NAP_GetNextID(hObjectAggregate, ulID)) != 0)
    {
        eRet = pfFunc (hObjectAggregate, ulID, pstUserHandle);
        if (eRet != BS_WALK_CONTINUE)
        {
            return;
        }
    }
}

UINT64 OBJECT_GetNextID(IN HANDLE hObjectAggregate, IN UINT64 uiCurrentID)
{
    return NAP_GetNextID(hObjectAggregate, uiCurrentID);
}

UINT OBJECT_GetCount(IN HANDLE hObjectAggregate)
{
    return NAP_GetCount(hObjectAggregate);
}

