/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-8-29
* Description: 
* History:     
******************************************************************************/


#define RETCODE_FILE_NUM RETCODE_FILE_NUM_OBJECT
    
#include "bs.h"

#include "utl/darray_utl.h"
#include "utl/kv_utl.h"
#include "utl/nap_utl.h"
#include "utl/object_utl.h"


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


HANDLE OBJECT_CreateAggregate(OBJECT_PARAM_S *p)
{
    NAP_PARAM_S param = {0};

    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = p->uiMaxNum;
    param.uiNodeSize = sizeof(OBJECT_S) + p->uiObjSize;
    param.memcap = p->memcap;

    return NAP_Create(&param);
}

VOID OBJECT_DestroyAggregate(IN HANDLE hAggregate)
{
    if (hAggregate == 0) {
        return;
    }

    OBJECT_FreeAllObjects(hAggregate);

    NAP_Destory(hAggregate);

    return;
}

VOID OBJECT_EnableSeq(IN HANDLE hAggregate, IN UINT64 ulMask, IN UINT uiCount)
{
    NAP_EnableSeq(hAggregate, ulMask, 1);
}

VOID * OBJECT_NewObject(IN HANDLE hAggregate)
{
    OBJECT_S *pstInnerObj;

    if (hAggregate == 0) {
        return 0;
    }

    pstInnerObj = NAP_ZAlloc(hAggregate);
    if (pstInnerObj == NULL)
    {
        return 0;
    }

    return _object_Inner2User(pstInnerObj);
}

VOID OBJECT_FreeObject(IN HANDLE hAggregate, IN VOID *pUserObj)
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

    NAP_Free(hAggregate, pstInnerObj);

    return;
}

VOID OBJECT_FreeObjectByID(IN HANDLE hAggregate, IN UINT64 ulID)
{
    VOID *pUserObj;

    pUserObj = OBJECT_GetObjectByID(hAggregate, ulID);
    if (NULL == pUserObj)
    {
        return;
    }

    OBJECT_FreeObject(hAggregate, pUserObj);
}

VOID * OBJECT_GetObjectByID(IN HANDLE hAggregate, IN UINT64 ulID)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hAggregate, ulID);
    if (NULL == pstInnerObj)
    {
        return NULL;
    }

    return _object_Inner2User(pstInnerObj);
}

UINT64 OBJECT_GetIDByObject(IN HANDLE hAggregate, IN VOID *pUserObj)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(pUserObj);

    return NAP_GetIDByNode(hAggregate, pstInnerObj);
}

VOID OBJECT_FreeAllObjects(IN HANDLE hAggregate)
{
    UINT64 ulNodeId = 0;

    while ((ulNodeId = NAP_GetNextID(hAggregate, ulNodeId) != 0))
    {
        OBJECT_FreeObjectByID(hAggregate, ulNodeId);
    }

    return;
}

BS_STATUS OBJECT_SetProperty(IN VOID *pUserObj, IN UINT uiPropertyIndex, IN HANDLE hValue)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(pUserObj);

    if (NULL == pstInnerObj->hPropertys)
    {
        pstInnerObj->hPropertys = DARRAY_Create(0, 32);
        if (NULL == pstInnerObj->hPropertys)
        {
            RETURN(BS_NO_MEMORY);
        }
    }

    return DARRAY_Set(pstInnerObj->hPropertys, uiPropertyIndex, hValue);
}


BS_STATUS OBJECT_SetPropertyByID(HANDLE hAggregate, UINT64 id, UINT property_index, void *value)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hAggregate, id);
    if (NULL == pstInnerObj)
    {
        RETURN(BS_NO_SUCH);
    }

    return OBJECT_SetProperty(_object_Inner2User(pstInnerObj), property_index, value);
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
    IN HANDLE hAggregate,
    IN UINT64 ulObjectId,
    IN UINT uiPropertyIndex,
    OUT HANDLE *phValue
)
{
    OBJECT_S *pstInnerObj;

    *phValue = NULL;
    
    pstInnerObj = NAP_GetNodeByID(hAggregate, ulObjectId);
    if (NULL == pstInnerObj)
    {
        RETURN(BS_NO_SUCH);
    }

    return OBJECT_GetProperty(_object_Inner2User(pstInnerObj), uiPropertyIndex, phValue);
}

static HANDLE object_create_kv(HANDLE hAggregate)
{
    KV_PARAM_S kv_param = {0};
    kv_param.memcap = NAP_GetMemCap(hAggregate);

    return KV_Create(&kv_param);
}

int OBJECT_SetKeyValue(HANDLE hAggregate, void *obj, char *key, char *value)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(obj);

    if (NULL == pstInnerObj->hKv) {
        pstInnerObj->hKv = object_create_kv(hAggregate);
        if (NULL == pstInnerObj->hKv) {
            RETURN(BS_NO_MEMORY);
        }
    }

    return KV_SetKeyValue(pstInnerObj->hKv, key, value);
}

int OBJECT_SetKeyValueByID(HANDLE hAggregate, UINT64 id, char *key, char *value)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hAggregate, id);
    if (NULL == pstInnerObj) {
        RETURN(BS_NO_SUCH);
    }

    return OBJECT_SetKeyValue(hAggregate, _object_Inner2User(pstInnerObj), key, value);
}

char * OBJECT_GetKeyValue(void *obj, char *key)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = _object_User2Inner(obj);

    if (NULL == pstInnerObj->hKv) {
        return NULL;
    }

    return KV_GetKeyValue(pstInnerObj->hKv, key);
}

char * OBJECT_GetKeyValueByID(HANDLE hAggregate, UINT64 id, char *key)
{
    OBJECT_S *pstInnerObj;

    pstInnerObj = NAP_GetNodeByID(hAggregate, id);
    if (NULL == pstInnerObj) {
        return NULL;
    }

    return OBJECT_GetKeyValue(_object_Inner2User(pstInnerObj), key);
}

VOID OBJECT_Walk(IN HANDLE hAggregate, IN OBJECT_WALK_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 ulID = 0;
    int ret;

    while ((ulID = NAP_GetNextID(hAggregate, ulID)) != 0) {
        ret = pfFunc (hAggregate, ulID, pstUserHandle);
        if (ret < 0) {
            return;
        }
    }
}

UINT64 OBJECT_GetNextID(IN HANDLE hAggregate, IN UINT64 uiCurrentID)
{
    return NAP_GetNextID(hAggregate, uiCurrentID);
}

UINT OBJECT_GetCount(IN HANDLE hAggregate)
{
    return NAP_GetCount(hAggregate);
}

