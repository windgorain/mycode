
#include "bs.h"

#include "vnets_domain_inner.h"

typedef struct
{
    DLL_NODE_S stNode;
    UINT uiNodeID;
}_VNETS_DOMAIN_NODE_S;

static _VNETS_DOMAIN_NODE_S * vnets_domainnode_Find
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN UINT uiNodeID
)
{
    _VNETS_DOMAIN_NODE_S *pstNode;

    DLL_SCAN(&pstTbl->stListHead, pstNode)
    {
        if (pstNode->uiNodeID == uiNodeID)
        {
            return pstNode;
        }
    }

    return NULL;
}

BS_STATUS _VNETS_DomainNode_Init(IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl)
{
    DLL_INIT(&pstTbl->stListHead);

    return BS_OK;
}

VOID _VNETS_DomainNode_Uninit(IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl)
{
    _VNETS_DOMAIN_NODE_S *pstNode, *pstNodeTmp;
    
    DLL_SAFE_SCAN(&pstTbl->stListHead, pstNode, pstNodeTmp)
    {
        DLL_DEL(&pstTbl->stListHead, pstNode);
        MEM_Free(pstNode);
    }

    return;
}

BS_STATUS _VNETS_DomainNode_Add
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN UINT uiNodeID
)
{
    _VNETS_DOMAIN_NODE_S *pstNode;

    BS_DBGASSERT(NULL != pstTbl);

    if (NULL != vnets_domainnode_Find(pstTbl, uiNodeID))
    {
        return BS_OK;
    }

    pstNode = MEM_ZMalloc(sizeof(_VNETS_DOMAIN_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->uiNodeID = uiNodeID;

    DLL_ADD(&pstTbl->stListHead, pstNode);

    return BS_OK;
}

VOID _VNETS_DomainNode_Del
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN UINT uiNodeID
)
{
    _VNETS_DOMAIN_NODE_S *pstNode;

    pstNode = vnets_domainnode_Find(pstTbl, uiNodeID);

    if (NULL != pstNode)
    {
        DLL_DEL(&pstTbl->stListHead, pstNode);
        MEM_Free(pstNode);
    }
}

UINT _VNETS_DomainNode_GetCount(IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl)
{
    return DLL_COUNT(&pstTbl->stListHead);
}

UINT _VNETS_DomainNode_GetNext
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN UINT uiCurrentNodeID
)
{
    _VNETS_DOMAIN_NODE_S *pstNode;
    UINT uiNextId = 0;

    DLL_SCAN(&pstTbl->stListHead, pstNode)
    {
        if (uiCurrentNodeID < pstNode->uiNodeID)
        {
            if ((uiNextId == 0) || (uiNextId > pstNode->uiNodeID))
            {
                uiNextId = pstNode->uiNodeID;
            }
        }
    }

    return uiNextId;
}

VOID _VNETS_DomainNode_Walk
(
    IN _VNETS_DOMAIN_NODE_TBL_S *pstTbl,
    IN PF_VNETS_DOMAIN_NODE_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    _VNETS_DOMAIN_NODE_S *pstNode, *pstNodeTmp;
    
    DLL_SAFE_SCAN(&pstTbl->stListHead, pstNode, pstNodeTmp)
    {
        pfFunc(pstNode->uiNodeID, hUserHandle);
    }
}


