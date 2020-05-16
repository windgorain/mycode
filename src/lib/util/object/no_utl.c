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

#define _NO_HASH_BUCKETS_NUM 1024

typedef struct
{
    HASH_NODE_S stHashNode;
    CHAR *pcName;
}_NO_NODE_S;

typedef struct
{
    HANDLE hObjectAggregate; /* 未命名对象集合 */
    HASH_HANDLE hHashTbl;
}_NO_INSTANCE_S;

static INT _no_CmpHashNode
(
    IN _NO_NODE_S * pstHashNode1,
    IN _NO_NODE_S * pstHadhNode2
)
{
    return strcmp(pstHashNode1->pcName, pstHadhNode2->pcName);
}

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
    HASH_Del(pstInstance->hHashTbl, pstNode);

    if (NULL != pstNode->pcName)
    {
        MEM_RcuFree(pstNode->pcName);
    }

    OBJECT_FreeObject(pstInstance->hObjectAggregate, pstNode);
}

static UINT _no_GetHashIndex(IN VOID *pstHashNode)
{
    _NO_NODE_S *pstNode = (_NO_NODE_S*)pstHashNode;
    CHAR *pcChar;
    UINT ulIndex = 0;

    pcChar = pstNode->pcName;

    while (*pcChar != '\0')
    {
        ulIndex += *pcChar;
        pcChar ++;
    }

    return ulIndex;
}

NO_HANDLE NO_CreateAggregate
(
    IN UINT uiMaxNum/* 0表示不限制 */,
    IN UINT uiObjSize/* 对象大小 */,
    IN UINT uiFlag  /* OBJECT_FLAG_XXX */ 
)
{
    _NO_INSTANCE_S *pstInstance;

    pstInstance = MEM_ZMalloc(sizeof(_NO_INSTANCE_S));
    if (NULL == pstInstance)
    {
        return NULL;
    }

    pstInstance->hObjectAggregate = OBJECT_CreateAggregate(uiMaxNum, uiObjSize + sizeof(_NO_NODE_S), uiFlag);
    if (NULL == pstInstance->hObjectAggregate)
    {
        MEM_Free(pstInstance);
        return NULL;
    }

    pstInstance->hHashTbl = HASH_CreateInstance(_NO_HASH_BUCKETS_NUM, _no_GetHashIndex);
    if (NULL == pstInstance->hHashTbl)
    {
        OBJECT_DestroyAggregate(pstInstance->hObjectAggregate);
        MEM_Free(pstInstance);
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

    if (NULL != pstInstance->hObjectAggregate)
    {
        while (0 != (ulID = OBJECT_GetNextID(pstInstance->hObjectAggregate, ulID)))
        {
            pstNode = OBJECT_GetObjectByID(pstInstance->hObjectAggregate, ulID);
            _no_FreeNode(pstInstance, pstNode);
        }
    }

    if (NULL != pstInstance->hHashTbl)
    {
        HASH_DestoryInstance(pstInstance->hHashTbl);
    }

    MEM_Free(pstInstance);
}

VOID NO_EnableSeq(IN NO_HANDLE hAggregate, IN UINT64 ulMask, IN UINT uiCount)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    OBJECT_EnableSeq(pstInstance->hObjectAggregate, ulMask, uiCount);
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
    pcNameTmp = MEM_RcuMalloc(uiLen + 1);
    if (NULL == pcNameTmp)
    {
        return NULL;
    }

    TXT_Strlcpy(pcNameTmp, pcName, uiLen + 1);

    pstNode = OBJECT_NewObject(pstInstance->hObjectAggregate);
    if (NULL == pstNode)
    {
        MEM_RcuFree(pcNameTmp);
        return NULL;
    }
    pstNode->pcName = pcNameTmp;

    HASH_Add(pstInstance->hHashTbl, pstNode);

    return _no_Node2Object(pstNode);
}

/* 自动在name后面添加Index */
VOID * NO_NewObjectForNameIndex(IN NO_HANDLE hAggregate, IN CHAR *pcNamePrefix /* 名称前缀 */)
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

    pstNode = OBJECT_NewObject(pstInstance->hObjectAggregate);
    if (NULL == pstNode)
    {
        return NULL;
    }
    
    uiSize = strlen(pcNamePrefix) + 16;

    pcNameTmp = MEM_Malloc(uiSize);
    if (NULL == pcNameTmp)
    {
        OBJECT_FreeObject(pstInstance->hObjectAggregate, pstNode);
		return NULL;
    }
    snprintf(pcNameTmp, uiSize, "%s%lld", pcNamePrefix, OBJECT_GetIDByObject(pstInstance->hObjectAggregate, pstNode));
    pstNode->pcName = pcNameTmp;

    HASH_Add(pstInstance->hHashTbl, pstNode);

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

VOID NO_FreeObjectByName(IN NO_HANDLE hAggregate, IN CHAR *pcName)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(hAggregate, pcName);
    if (NULL != pNode)
    {
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

    return OBJECT_GetIDByObject(pstInstance->hObjectAggregate, pstNode);
}

CHAR * NO_GetName(IN VOID * pObject)
{
    _NO_NODE_S *pstNode;

    if (NULL == pObject)
    {
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

    pstNode = OBJECT_GetObjectByID(pstInstance->hObjectAggregate, ulID);
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
    _NO_NODE_S stNode;

    if ((NULL == hAggregate) || (NULL == pcName))
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    stNode.pcName = pcName;

    pstNode = (_NO_NODE_S *)HASH_Find(pstInstance->hHashTbl,
        (PF_HASH_CMP_FUNC)_no_CmpHashNode, (HASH_NODE_S *)&stNode);
    if (NULL == pstNode)
    {
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

    pstNode = OBJECT_GetObjectByID(pstInstance->hObjectAggregate, ulID);
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

BS_STATUS NO_SetProperty(IN VOID *pObject, IN UINT uiPropertyIndex, IN HANDLE hValue)
{
    _NO_NODE_S *pstNode;

    pstNode = _no_Object2Node(pObject);

    return OBJECT_SetProperty(pstNode, uiPropertyIndex, hValue);
}

BS_STATUS NO_SetPropertyByID
(
    IN NO_HANDLE hAggregate,
    IN UINT64 ulObjectId,
    IN UINT uiPropertyIndex,
    IN HANDLE hValue
)
{
    _NO_INSTANCE_S *pstInstance = hAggregate;

    return OBJECT_SetPropertyByID(pstInstance->hObjectAggregate, ulObjectId, uiPropertyIndex, hValue);
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

    return OBJECT_GetPropertyByID(pstInstance->hObjectAggregate, ulObjectId, uiPropertyIndex, phValue);
}

BS_STATUS NO_SetKeyValue(IN NO_HANDLE hAggregate, IN VOID *pObject, IN CHAR *pcKey, IN CHAR *pcValue)
{
    _NO_NODE_S *pstNode;
    _NO_INSTANCE_S *pstInstance = hAggregate;

    pstNode = _no_Object2Node(pObject);

    return OBJECT_SetKeyValue(pstInstance->hObjectAggregate, pstNode, pcKey, pcValue);
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

    return OBJECT_SetKeyValueByID(pstInstance->hObjectAggregate, ulObjectId, pcKey, pcValue);
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

    return OBJECT_GetKeyValueByID(pstInstance->hObjectAggregate, ulObjectId, pcKey);
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
    
    return OBJECT_GetNextID(pstInstance->hObjectAggregate, ulCurrentID);
}

/* 根据名字字典序进行获取下一个 */
CHAR * NO_GetNextName(IN NO_HANDLE hAggregate, IN CHAR *pcCurrentName/* NULL或""表示获取第一个 */)
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

    return OBJECT_GetCount(pstInstance->hObjectAggregate);
}

