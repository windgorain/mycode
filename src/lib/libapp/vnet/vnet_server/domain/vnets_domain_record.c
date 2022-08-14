
#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/txt_utl.h"

#include "../inc/vnets_domain.h"
#include "../inc/vnets_rcu.h"

#include "vnets_domain_inner.h"

#define _VNETS_DOMAIN_RECORD_HASH_BUCKET_NUM 512

typedef struct
{
    HASH_NODE_S stHashNode;
    RCU_NODE_S stRcuNode;
    _VNETS_DOMAIN_NODE_TBL_S stSesIDList;
    VNETS_DOMAIN_RECORD_S stParam;
}VNETS_DOMAIN_RECORD_NODE_S;


static HASH_HANDLE g_hVnetsDomainRecordHashHandle = NULL;

static UINT vnets_domainrecord_HashIndex(IN VOID *pstHashNode)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode = pstHashNode;

    return pstNode->stParam.uiDomainID;
}

static INT vnets_domainrecord_CmpNode
(
    IN VNETS_DOMAIN_RECORD_NODE_S * pstHashNode1,
    IN VNETS_DOMAIN_RECORD_NODE_S * pstHashNode2
)
{
    return pstHashNode2->stParam.uiDomainID - pstHashNode1->stParam.uiDomainID;
}

static VNETS_DOMAIN_RECORD_NODE_S * vnets_domainrecord_Find(IN UINT uiDomainID)
{
    VNETS_DOMAIN_RECORD_NODE_S stToFind;

    stToFind.stParam.uiDomainID = uiDomainID;

    return (VNETS_DOMAIN_RECORD_NODE_S*)HASH_Find(g_hVnetsDomainRecordHashHandle,
                (PF_HASH_CMP_FUNC)vnets_domainrecord_CmpNode, (HASH_NODE_S*)&stToFind);
}

static inline BS_STATUS vnets_domainrecord_Add
(
    IN UINT uiDomainID,
    IN CHAR *pcDomainName,
    IN VNETS_DOMAIN_TYPE_E eType,
    IN UINT uiNodeID
)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(VNETS_DOMAIN_RECORD_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->stParam.uiDomainID = uiDomainID;
    pstNode->stParam.eType = eType;
    TXT_Strlcpy(pstNode->stParam.szDomainName, pcDomainName, sizeof(pstNode->stParam.szDomainName));
    _VNETS_DomainNode_Init(&pstNode->stSesIDList);
    if (uiNodeID != 0)
    {
        _VNETS_DomainNode_Add(&pstNode->stSesIDList, uiNodeID);
    }

    HASH_Add(g_hVnetsDomainRecordHashHandle, (HASH_NODE_S*)pstNode);

    _VNETS_DomainEvent_Issu(&pstNode->stParam, VNET_DOMAIN_EVENT_AFTER_CREATE);

    return BS_OK;
}

static BS_WALK_RET_E _vnets_domainrecord_WalkDomain
(
    IN HASH_HANDLE hHashId,
    IN VOID *pNode,
    IN VOID * pUserHandle
)
{
    USER_HANDLE_S *pstUserHandle = (USER_HANDLE_S*)pUserHandle;
    VNETS_DOMAIN_RECORD_NODE_S *pstNode = pNode;
    PF_VNETS_DOMAIN_WALK_FUNC pfFunc = pstUserHandle->ahUserHandle[0];

    return pfFunc(&pstNode->stParam, pstUserHandle->ahUserHandle[1]);
}

static VOID vnets_domainrecord_RcuDel(IN VOID *pstRcuNode)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode = BS_ENTRY(pstRcuNode, stRcuNode, VNETS_DOMAIN_RECORD_NODE_S);

    _VNETS_DomainEvent_Issu(&pstNode->stParam, VNET_DOMAIN_EVENT_PRE_DESTROY);

    MEM_Free(pstNode);
}

static VOID vnets_domainrecord_Del(IN VNETS_DOMAIN_RECORD_NODE_S *pstNode)
{
    _VNETS_DomainNode_Uninit(&pstNode->stSesIDList);
    HASH_Del(g_hVnetsDomainRecordHashHandle, (HASH_NODE_S*)pstNode);
    VNETS_RCU_Free(&pstNode->stRcuNode, vnets_domainrecord_RcuDel);
}

BS_STATUS _VNETS_DomainRecord_Init()
{
    HASH_HANDLE hHash;
    
    hHash = HASH_CreateInstance(NULL, _VNETS_DOMAIN_RECORD_HASH_BUCKET_NUM, vnets_domainrecord_HashIndex);
    if (hHash == NULL)
    {
        return BS_NO_MEMORY;
    }

    g_hVnetsDomainRecordHashHandle = hHash;

    return BS_OK;
}

BS_STATUS _VNETS_DomainRecord_Add
(
    IN UINT uiDomainID,
    IN CHAR *pcDomainName,
    IN VNETS_DOMAIN_TYPE_E eType,
    IN UINT uiNodeID
)
{
    BS_STATUS eRet;

    if (uiDomainID == 0)
    {
        return BS_BAD_PARA;
    }
    
    eRet = vnets_domainrecord_Add(uiDomainID, pcDomainName, eType, uiNodeID);

    return eRet;
}

VOID _VNETS_DomainRecord_Del(IN UINT uiDomainID)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = vnets_domainrecord_Find(uiDomainID);
    if (pstNode != NULL)
    {
        vnets_domainrecord_Del(pstNode);
    }
}

VOID _VNETS_DomainRecord_AddNode
(
    IN UINT uiDomainID,
    IN UINT uiNodeID
)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    if (0 == uiNodeID)
    {
        return;
    }

    pstNode = vnets_domainrecord_Find(uiDomainID);
    if (pstNode != NULL)
    {
        _VNETS_DomainNode_Add(&pstNode->stSesIDList, uiNodeID);
    }
}

VOID _VNETS_DomainRecord_DelNode(IN UINT uiDomainID, IN UINT uiNodeID)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = vnets_domainrecord_Find(uiDomainID);
    if (pstNode == NULL)
    {
        return;
    }

    if (uiNodeID != 0)
    {
        _VNETS_DomainNode_Del(&pstNode->stSesIDList, uiNodeID);
    }

    if (_VNETS_DomainNode_GetCount(&pstNode->stSesIDList) == 0)
    {
        _VNETS_DomainId_FreeID(uiDomainID);
        _VNETS_DomainNIM_Del(pstNode->stParam.szDomainName);
        vnets_domainrecord_Del(pstNode);
    }
}

BOOL_T _VNETS_DomainRecord_IsExist(IN UINT uiDomainID)
{
    if (NULL == vnets_domainrecord_Find(uiDomainID))
    {
        return FALSE;
    }

    return TRUE;
}

BS_STATUS _VNETS_DomainRecord_SetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    IN HANDLE hValue
)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = vnets_domainrecord_Find(uiDomainId);

    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    pstNode->stParam.ahProperty[ePropertyIndex] = hValue;

    return BS_OK;
}


BS_STATUS _VNETS_DomainRecord_GetProperty
(
    IN UINT uiDomainId,
    IN VNETS_DOMAIN_PROPERTY_E ePropertyIndex,
    OUT HANDLE *phValue
)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = vnets_domainrecord_Find(uiDomainId);

    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    *phValue = pstNode->stParam.ahProperty[ePropertyIndex];

    return BS_OK;
}

BS_STATUS _VNETS_DomainRecord_GetAllProperty
(
    IN UINT uiDomainID,
    OUT HANDLE *phValue
)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;
    UINT i;

    pstNode = vnets_domainrecord_Find(uiDomainID);

    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    for (i=0; i<VNET_DOMAIN_PROPERTY_MAX; i++)
    {
        phValue[i] = pstNode->stParam.ahProperty[i];
    }

    return BS_OK;
}

BS_STATUS _VNETS_DomainRecord_GetDomainName
(
    IN UINT uiDomainId,
    OUT CHAR szDomainName[VNET_CONF_MAX_DOMAIN_NAME_LEN + 1]
)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    szDomainName[0] = '\0';

    pstNode = vnets_domainrecord_Find(uiDomainId);

    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    TXT_Strlcpy(szDomainName, pstNode->stParam.szDomainName, VNET_CONF_MAX_DOMAIN_NAME_LEN + 1);

    return BS_OK;
}

UINT _VNETS_DomainRecord_GetSesCount(IN UINT uiDomainId)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = vnets_domainrecord_Find(uiDomainId);

    if (NULL == pstNode)
    {
        return 0;
    }

    return _VNETS_DomainNode_GetCount(&pstNode->stSesIDList);
}

UINT _VNETS_DomainRecord_GetNextNode(IN UINT uiDomainId, IN UINT uiCurrentNodeId/* 如果为0,则表示得到第一个 */)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = vnets_domainrecord_Find(uiDomainId);

    if (NULL == pstNode)
    {
        return 0;
    }
    
    return _VNETS_DomainNode_GetNext(&pstNode->stSesIDList, uiCurrentNodeId);
}

VOID _VNETS_DomainRecord_WalkDomain(IN PF_VNETS_DOMAIN_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;
    
    HASH_Walk(g_hVnetsDomainRecordHashHandle, _vnets_domainrecord_WalkDomain, &stUserHandle);
}

VOID _VNETS_DomainRecord_WalkNode
(
    IN UINT uiDomainId,
    IN PF_VNETS_DOMAIN_NODE_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    VNETS_DOMAIN_RECORD_NODE_S *pstNode;

    pstNode = vnets_domainrecord_Find(uiDomainId);

    if (NULL == pstNode)
    {
        return;
    }
    
    _VNETS_DomainNode_Walk(&pstNode->stSesIDList, pfFunc, hUserHandle);
}


