/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-5-23
* Description: 树形组织关系
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/kv_utl.h"
#include "utl/nap_utl.h"
#include "utl/tree_utl.h"
#include "utl/org_tree.h"

#define USER_TREE_MAX_ORG_NAME_LEN 127

typedef struct
{
    TREE_NODE_S stTreeNode;
    CHAR szOrgName[USER_TREE_MAX_ORG_NAME_LEN + 1];
    KV_HANDLE hKvList;   
}ORG_TREE_ORG_S;


typedef struct
{
    NAP_HANDLE hOrgNap;
    ORG_TREE_ORG_S *pstRoot;
}ORG_TREE_S;

static ORG_TREE_ORG_S * orgtree_GetParent(IN ORG_TREE_ORG_S *pstOrg)
{
    return (ORG_TREE_ORG_S*)TREE_GetParent(&pstOrg->stTreeNode);
}

static ORG_TREE_ORG_S * orgtree_GetOrgByID(IN ORG_TREE_S *pstUserTree, IN UINT64 uiOrgID)
{
    if (uiOrgID == 0)
    {
        NULL;
    }

    return NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
}

static VOID orgtree_AddChildOrg
(
    IN ORG_TREE_S *pstUserTree,
    IN ORG_TREE_ORG_S *pstOrg,
    IN ORG_TREE_ORG_S *pstChildOrg
)
{
    UINT64 uiChildOrgID = NAP_GetIDByNode(&pstUserTree->hOrgNap, pstChildOrg);
    UINT64 uiIdTmp;
    ORG_TREE_ORG_S *pstOrgTmp;
    ORG_TREE_ORG_S *pstOrgFind = NULL;

    TREE_SCAN_SUBNODE(&pstOrg->stTreeNode, pstOrgTmp)
    {
        uiIdTmp = NAP_GetIDByNode(&pstUserTree->hOrgNap, pstOrgTmp);
        if (uiIdTmp > uiChildOrgID)
        {
            pstOrgFind = pstOrgTmp;
            break;
        }
    }

    DLL_INSERT_BEFORE(&pstOrg->stTreeNode.stChildNode, pstChildOrg, pstOrgFind);
}

static UINT64 orgtree_AddOrg(IN ORG_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcOrgName, IN UINT64 uiSpecID)
{
    ORG_TREE_S *pstUserTree = hUserTree;
    ORG_TREE_ORG_S *pstOrg;
    ORG_TREE_ORG_S *pstParentOrg;

    BS_DBGASSERT(uiParentOrgID != 0);

    pstParentOrg = orgtree_GetOrgByID(pstUserTree, uiParentOrgID);
    if (NULL == pstParentOrg)
    {
        return 0;
    }

    pstOrg = NAP_ZAllocByID(pstUserTree->hOrgNap, uiSpecID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    pstOrg->hKvList = KV_Create(0);
    if (NULL == pstOrg->hKvList)
    {
        NAP_Free(pstUserTree->hOrgNap, pstOrg);
        return 0;
    }

    TXT_Strlcpy(pstOrg->szOrgName, pcOrgName, sizeof(pstOrg->szOrgName));
    TREE_NodeInit(&pstOrg->stTreeNode);

    if (NULL != pstParentOrg)
    {
        orgtree_AddChildOrg(pstUserTree, pstParentOrg, pstOrg);
    }

    return NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrg);
}

static VOID orgtree_DeleteOrg(IN ORG_TREE_S *pstUserTree, IN ORG_TREE_ORG_S *pstOrg)
{
    ORG_TREE_ORG_S *pstOrgChild;
    ORG_TREE_ORG_S *pstOrgTmp;

    
    DLL_SAFE_SCAN(&pstOrg->stTreeNode.stChildNode, pstOrgChild, pstOrgTmp)
    {
        orgtree_DeleteOrg(pstUserTree, pstOrgChild);
    }

    TREE_RemoveNode(&pstOrg->stTreeNode);

    KV_Destory(pstOrg->hKvList);
    NAP_Free(pstUserTree->hOrgNap, pstOrg);
}

ORG_TREE_HANDLE OrgTree_Create()
{
    ORG_TREE_S *pstTree;
    NAP_PARAM_S param = {0};

    pstTree = MEM_ZMalloc(sizeof(ORG_TREE_S));
    if (NULL == pstTree)
    {
        return NULL;
    }

    param.enType = NAP_TYPE_HASH;
    param.uiNodeSize = sizeof(ORG_TREE_ORG_S);
    pstTree->hOrgNap = NAP_Create(&param);
    if (NULL == pstTree->hOrgNap)
    {
        OrgTree_Destroy(pstTree);
        return NULL;
    }

    return pstTree;
}

VOID OrgTree_Destroy(IN ORG_TREE_HANDLE hOrgTree)
{
    ORG_TREE_S *pstTree = hOrgTree;

    if (pstTree->pstRoot != NULL)
    {
        orgtree_DeleteOrg(hOrgTree, pstTree->pstRoot);
    }

    if (NULL != pstTree->hOrgNap)
    {
        NAP_Destory(pstTree->hOrgNap);
    }

    MEM_Free(pstTree);
}

UINT64 OrgTree_AddOrg(IN ORG_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcOrgName)
{
    return orgtree_AddOrg(hUserTree, uiParentOrgID, pcOrgName, 0);
}

BS_STATUS OrgTree_DeleteOrg(IN ORG_TREE_HANDLE hOrgTree, IN UINT64 uiOrgID)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;

    if (uiOrgID == 0)
    {
        return BS_BAD_PARA;
    }

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return BS_OK;
    }

    orgtree_DeleteOrg(pstUserTree, pstOrg);

    return BS_OK;
}

CHAR * OrgTree_GetOrgName(IN ORG_TREE_HANDLE hOrgTree, IN UINT64 uiOrgID)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return NULL;
    }

    return pstOrg->szOrgName;
}

BS_STATUS OrgTree_SetOrgParent(IN ORG_TREE_HANDLE hOrgTree, IN UINT64 uiOrgID, IN UINT64 uiParentOrgID)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;
    ORG_TREE_ORG_S *pstParentOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return BS_NO_SUCH;
    }

    if (pstOrg == pstUserTree->pstRoot)
    {
        return BS_NOT_SUPPORT;
    }

    pstParentOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiParentOrgID);
    if (NULL == pstParentOrg)
    {
        return BS_NO_SUCH;
    }

    TREE_RemoveNode(&pstOrg->stTreeNode);

    orgtree_AddChildOrg(pstUserTree, pstParentOrg, pstOrg);

    return BS_OK;
}

UINT64 OrgTree_GetOrgParent(IN ORG_TREE_HANDLE hOrgTree, IN UINT64 uiOrgID)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;
    ORG_TREE_ORG_S *pstOrgParent;

    if (uiOrgID == 0)
    {
        return 0;
    }

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    pstOrgParent = orgtree_GetParent(pstOrg);
    if (NULL == pstOrgParent)
    {
        return 0;
    }

    return NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrgParent);
}

BS_STATUS OrgTree_SetOrgKeyValue
(
    IN ORG_TREE_HANDLE hOrgTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcKey,
    IN CHAR *pcValue
)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return BS_NO_SUCH;
    }

    return KV_SetKeyValue(pstOrg->hKvList, pcKey, pcValue);
}

CHAR * OrgTree_GetOrgKeyValue
(
    IN ORG_TREE_HANDLE hOrgTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcKey
)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return NULL;
    }

    return KV_GetKeyValue(pstOrg->hKvList, pcKey);
}

UINT64 OrgTree_GetNextChildOrg
(
    IN ORG_TREE_HANDLE hOrgTree,
    IN UINT64 uiOrgID,
    IN UINT64 uiCurrentChildOrgID
)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;
    ORG_TREE_ORG_S *pstOrgTmp;
    UINT64 uiTmpID;
    UINT64 uiNextChildID = 0;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    TREE_SCAN_SUBNODE(&pstOrg->stTreeNode, pstOrgTmp)
    {
        uiTmpID = NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrgTmp);
        if (uiTmpID > uiCurrentChildOrgID)
        {
            uiNextChildID = uiTmpID;
            break;
        }
    }

    return uiNextChildID;
}

UINT64 OrgTree_FindChildOrgByName
(
    IN ORG_TREE_HANDLE hOrgTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcOrgName
)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    UINT64 uiFoundID = 0;
    ORG_TREE_ORG_S *pstOrg;
    ORG_TREE_ORG_S *pstOrgTmp;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    TREE_SCAN_SUBNODE(&pstOrg->stTreeNode, pstOrgTmp)
    {
        if (strcmp(pstOrgTmp->szOrgName, pcOrgName) == 0)
        {
            uiFoundID = NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrgTmp);
            break;
        }
    }

    return uiFoundID;
}


UINT64 OrgTree_GetNextOrgByIndexOrder(IN ORG_TREE_HANDLE hOrgTree, IN UINT64 uiCurrentOrgID)
{
    ORG_TREE_S *pstUserTree = hOrgTree;

    return NAP_GetNextID(pstUserTree->hOrgNap, uiCurrentOrgID);
}

static int orgtree_WalkOrgByDeepOrder
(
    IN ORG_TREE_S *pstUserTree,
    IN ORG_TREE_ORG_S *pstOrg,
    IN PF_OrgTree_Walk_Func pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    ORG_TREE_ORG_S *pstOrg1;
    ORG_TREE_ORG_S *pstOrg2;
    int ret = 0;
    
    ret = pfWalkFunc(pstUserTree, NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrg), hUserHandle);
    if (ret < 0) {
        return ret;
    }

    DLL_SAFE_SCAN(&pstOrg->stTreeNode.stChildNode, pstOrg1, pstOrg2) {
        ret = orgtree_WalkOrgByDeepOrder(pstUserTree, pstOrg1, pfWalkFunc, hUserHandle);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}


int OrgTree_WalkOrgByDeepOrder
(
    IN ORG_TREE_HANDLE hOrgTree,
    IN PF_OrgTree_Walk_Func pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    ORG_TREE_S *pstUserTree = hOrgTree;
    ORG_TREE_ORG_S *pstOrg;

    pstOrg = pstUserTree->pstRoot;
    if (NULL == pstOrg) {
        return 0;
    }

    return orgtree_WalkOrgByDeepOrder(pstUserTree, pstOrg, pfWalkFunc, hUserHandle);
}



