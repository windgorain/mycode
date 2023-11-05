/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-7-3
* Description: Named Object
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/object_utl.h"
#include "utl/hash_utl.h"
#include "utl/txt_utl.h"
#include "utl/nap_utl.h"
#include "utl/map_utl.h"
#include "utl/mem_cap.h"

typedef struct
{
    HASH_NODE_S stHashNode;
    CHAR *pcName;
}_NO_NODE_S;

typedef struct
{
    HANDLE hAggregate; 
    MAP_HANDLE name_map;
    void *memcap;
}_NO_INSTANCE_S;

static inline _NO_NODE_S * _no_Object2Node(IN VOID * pObject)
{
    return (_NO_NODE_S *)((CHAR*)pObject - sizeof(_NO_NODE_S));
}

static inline VOID * _no_Node2Object(IN _NO_NODE_S * pstNode)
{
    return (pstNode + 1);
}

static VOID _no_FreeNode(IN _NO_INSTANCE_S *pstInstance, IN _NO_NODE_S *pstNode)
{
    MAP_Del(pstInstance->name_map, pstNode->pcName, strlen(pstNode->pcName));

    if (NULL != pstNode->pcName) {
        MemCap_Free(pstInstance->memcap, pstNode->pcName);
    }

    OBJECT_FreeObject(pstInstance->hAggregate, pstNode);
}

NO_HANDLE NO_CreateAggregate(OBJECT_PARAM_S *p)
{
    _NO_INSTANCE_S *pstInstance;

    pstInstance = MemCap_ZMalloc(p->memcap, sizeof(_NO_INSTANCE_S));
    if (NULL == pstInstance) {
        return NULL;
    }
    pstInstance->memcap = p->memcap;

    OBJECT_PARAM_S param = *p;
    param.uiObjSize = p->uiObjSize + sizeof(_NO_NODE_S);

    pstInstance->hAggregate = OBJECT_CreateAggregate(&param);
    if (NULL == pstInstance->hAggregate) {
        MemCap_Free(p->memcap, pstInstance);
        return NULL;
    }

    pstInstance->name_map = MAP_HashCreate(NULL);
    if (NULL == pstInstance->name_map) {
        OBJECT_DestroyAggregate(pstInstance->hAggregate);
        MemCap_Free(p->memcap, pstInstance);
        return NULL;
    }

    return pstInstance;
}

VOID NO_DestroyAggregate(IN NO_HANDLE hAggregate)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;
    UINT64 ulID = 0;

    if (NULL == hAggregate)
    {
        return;
    }

    if (NULL != pstInstance->hAggregate) {
        while (0 != (ulID = OBJECT_GetNextID(pstInstance->hAggregate, ulID))) {
            pstNode = OBJECT_GetObjectByID(pstInstance->hAggregate, ulID);
            _no_FreeNode(pstInstance, pstNode);
        }
    }

    if (NULL != pstInstance->name_map) {
        MAP_Destroy(pstInstance->name_map, NULL, NULL);
    }

    MemCap_Free(pstInstance->memcap, pstInstance);
}

VOID NO_EnableSeq(IN NO_HANDLE hAggregate, IN UINT64 ulMask, IN UINT uiCount)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    OBJECT_EnableSeq(pstInstance->hAggregate, ulMask, uiCount);
}

VOID * NO_NewObject(IN NO_HANDLE hAggregate, IN CHAR *pcName)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;
    CHAR *pcNameTmp;
    UINT uiLen;

    if ((NULL == hAggregate) || (NULL == pcName))
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    uiLen = strlen(pcName);
    pcNameTmp = MemCap_Malloc(pstInstance->memcap, uiLen + 1);
    if (NULL == pcNameTmp)
    {
        return NULL;
    }

    TXT_Strlcpy(pcNameTmp, pcName, uiLen + 1);

    pstNode = OBJECT_NewObject(pstInstance->hAggregate);
    if (NULL == pstNode)
    {
        MemCap_Free(pstInstance->memcap, pcNameTmp);
        return NULL;
    }
    pstNode->pcName = pcNameTmp;

    MAP_Add(pstInstance->name_map, pstNode->pcName, strlen(pstNode->pcName), pstNode, 0);

    return _no_Node2Object(pstNode);
}


VOID * NO_NewObjectForNameIndex(IN NO_HANDLE hAggregate, IN CHAR *pcNamePrefix )
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;
    CHAR *pcNameTmp;
    UINT uiSize;

    if (NULL == hAggregate)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    if (NULL == pcNamePrefix)
    {
        pcNamePrefix = "";
    }

    pstNode = OBJECT_NewObject(pstInstance->hAggregate);
    if (NULL == pstNode)
    {
        return NULL;
    }
    
    uiSize = strlen(pcNamePrefix) + 16;

    pcNameTmp = MemCap_Malloc(pstInstance->memcap, uiSize);
    if (NULL == pcNameTmp)
    {
        OBJECT_FreeObject(pstInstance->hAggregate, pstNode);
		return NULL;
    }
    snprintf(pcNameTmp, uiSize, "%s%lld", pcNamePrefix, OBJECT_GetIDByObject(pstInstance->hAggregate, pstNode));
    pstNode->pcName = pcNameTmp;

    MAP_Add(pstInstance->name_map, pstNode->pcName, strlen(pstNode->pcName), pstNode, 0);

    return _no_Node2Object(pstNode);
}

UINT64 NO_NewObjectID(IN NO_HANDLE hAggregate, IN CHAR *pcName)
{
    VOID * pUsrObj;

    pUsrObj = NO_NewObject(hAggregate, pcName);
    if (NULL == pUsrObj)
    {
        return 0;
    }

    return NO_GetObjectID(hAggregate, pUsrObj);
}

VOID NO_FreeObject(IN NO_HANDLE hAggregate, IN VOID *pObject)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;

    if ((NULL == hAggregate) || (NULL == pObject))
    {
        BS_DBGASSERT(0);
        return;
    }

    pstNode = _no_Object2Node(pObject);

    _no_FreeNode(pstInstance, pstNode);
}

VOID NO_FreeObjectByID(IN NO_HANDLE hAggregate, IN UINT64 ulID)
{
    VOID *pObject;

    pObject = NO_GetObjectByID(hAggregate, ulID);
    if (NULL != pObject)
    {
        NO_FreeObject(hAggregate, pObject);
    }
}

VOID NO_FreeObjectByName(NO_HANDLE hAggregate, char *name)
{
    void *pNode;

    pNode = NO_GetObjectByName(hAggregate, name);
    if (NULL != pNode) {
        NO_FreeObject(hAggregate, pNode);
    }
}

UINT64 NO_GetObjectID(IN NO_HANDLE hAggregate, IN VOID * pObject)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;

    if ((NULL == hAggregate) || (NULL == pObject))
    {
        return 0;
    }

    pstNode = _no_Object2Node(pObject);

    return OBJECT_GetIDByObject(pstInstance->hAggregate, pstNode);
}

CHAR * NO_GetName(IN VOID * pObject)
{
    _NO_NODE_S *pstNode;

    if (NULL == pObject) {
        BS_DBGASSERT(0);
        return 0;
    }

    pstNode = _no_Object2Node(pObject);

    return pstNode->pcName;
}

VOID * NO_GetObjectByID(IN NO_HANDLE hAggregate, IN UINT64 ulID)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;

    if (NULL == hAggregate)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    pstNode = OBJECT_GetObjectByID(pstInstance->hAggregate, ulID);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return _no_Node2Object(pstNode);
}

VOID * NO_GetObjectByName(IN NO_HANDLE hAggregate, IN CHAR *pcName)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;

    if ((NULL == hAggregate) || (NULL == pcName))
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    pstNode = MAP_Get(pstInstance->name_map, pcName, strlen(pcName));
    if (NULL == pstNode) {
        return NULL;
    }

    return _no_Node2Object(pstNode);
}

CHAR * NO_GetNameByID(IN NO_HANDLE hAggregate, IN UINT64 ulID)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;
    _NO_NODE_S *pstNode;

    if (NULL == hAggregate)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    pstNode = OBJECT_GetObjectByID(pstInstance->hAggregate, ulID);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode->pcName;
}

UINT64 NO_GetIDByName(IN NO_HANDLE hAggregate, IN CHAR *pcName)
{
    VOID *pObject;
    
    pObject = NO_GetObjectByName(hAggregate, pcName);

    return NO_GetObjectID(hAggregate, pObject);
}

BS_STATUS NO_SetProperty(void *object, UINT property_index, void *value)
{
    _NO_NODE_S *pstNode;

    pstNode = _no_Object2Node(object);

    return OBJECT_SetProperty(pstNode, property_index, value);
}

BS_STATUS NO_SetPropertyByID(NO_HANDLE hAggregate, UINT64 id, UINT property_index, void *value)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    return OBJECT_SetPropertyByID(pstInstance->hAggregate, id, property_index, value);
}

BS_STATUS NO_GetProperty(IN VOID *pObject, IN UINT uiPropertyIndex, OUT HANDLE *phValue)
{
    _NO_NODE_S *pstNode;

    pstNode = _no_Object2Node(pObject);

    return OBJECT_GetProperty(pstNode, uiPropertyIndex, phValue);
}

BS_STATUS NO_GetPropertyByID
(
    IN NO_HANDLE hAggregate,
    IN UINT64 ulObjectId,
    IN UINT uiPropertyIndex,
    OUT HANDLE *phValue
)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    return OBJECT_GetPropertyByID(pstInstance->hAggregate, ulObjectId, uiPropertyIndex, phValue);
}

BS_STATUS NO_SetKeyValue(HANDLE hAggregate, void *obj, char *key, char *value)
{
    _NO_NODE_S *pstNode;
    _NO_INSTANCE_S *pstInstance = hAggregate;

    pstNode = _no_Object2Node(obj);

    return OBJECT_SetKeyValue(pstInstance->hAggregate, pstNode, key, value);
}

BS_STATUS NO_SetKeyValueByID
(
    IN NO_HANDLE hAggregate,
    IN UINT64 ulObjectId,
    IN CHAR *pcKey,
    IN CHAR *pcValue
)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    return OBJECT_SetKeyValueByID(pstInstance->hAggregate, ulObjectId, pcKey, pcValue);
}

CHAR * NO_GetKeyValue(IN VOID *pObject, IN CHAR *pcKey)
{
    _NO_NODE_S *pstNode;

    pstNode = _no_Object2Node(pObject);

    return OBJECT_GetKeyValue(pstNode, pcKey);
}

CHAR * NO_GetKeyValueByID
(
    IN NO_HANDLE hAggregate,
    IN UINT64 ulObjectId,
    IN CHAR *pcKey
)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    return OBJECT_GetKeyValueByID(pstInstance->hAggregate, ulObjectId, pcKey);
}

CHAR * NO_GetKeyValueByName
(
    IN NO_HANDLE hAggregate,
    IN CHAR *pcNodeName,
    IN CHAR *pcKey
)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(hAggregate, pcNodeName);
    if (NULL == pNode)
    {
        return NULL;
    }

    return NO_GetKeyValue(pNode, pcKey);
}

UINT64 NO_GetNextID(IN NO_HANDLE hAggregate, IN UINT64 ulCurrentID)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    if (NULL == hAggregate)
    {
        BS_DBGASSERT(0);
        return 0;
    }
    
    return OBJECT_GetNextID(pstInstance->hAggregate, ulCurrentID);
}


CHAR * NO_GetNextName(IN NO_HANDLE hAggregate, IN CHAR *pcCurrentName)
{
    UINT64 ulID = 0;
    CHAR *pcName;
    CHAR *pcFound = NULL;

    if (pcCurrentName == NULL)
    {
        pcCurrentName = "";
    }

    while((ulID = NO_GetNextID(hAggregate, ulID)) != 0)
    {
        pcName = NO_GetNameByID(hAggregate, ulID);
        if ((strcmp(pcName, pcCurrentName) > 0) && ((NULL == pcFound) || (strcmp(pcName, pcFound) < 0)))
        {
            pcFound = pcName;
        }
    }

    return pcFound;
}

UINT NO_GetCount(IN NO_HANDLE hAggregate)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    return OBJECT_GetCount(pstInstance->hAggregate);
}

