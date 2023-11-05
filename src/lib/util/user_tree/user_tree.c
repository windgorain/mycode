/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-4-21
* Description: 树形用户管理. 组织 - 用户关系
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/kv_utl.h"
#include "utl/nap_utl.h"
#include "utl/dc_utl.h"
#include "utl/dc_xml.h"
#include "utl/cjson.h"
#include "utl/user_tree.h"


#define USER_TREE_MAX_ORGANIZATION_NUM 1024
#define USER_TREE_MAX_USER_NUM 0xffff

#define USER_TREE_MAX_ORG_NAME_LEN 127

typedef struct
{
    DLL_NODE_S stLinkNode;
    CHAR szOrgName[USER_TREE_MAX_ORG_NAME_LEN + 1];
    VOID *pParentOrg;    
    KV_HANDLE hKvList;   
    DLL_HEAD_S stChildOrgList;
    DLL_HEAD_S stChildUserList;
}USER_TREE_ORG_S;

#define USER_TREE_MAX_USER_NAME_LEN 127

typedef struct
{
    DLL_NODE_S stLinkNode;
    CHAR szUserName[USER_TREE_MAX_USER_NAME_LEN + 1];
    USER_TREE_ORG_S *pstParentOrg;  
    KV_HANDLE hKvList;   
}USER_TREE_USER_S;

typedef struct
{
    NAP_HANDLE hOrgNap;
    NAP_HANDLE hUserNap;
    UINT64 uiRootOrgID;
}USER_TREE_S;

static USER_TREE_ORG_S * usertree_GetOrgByID(IN USER_TREE_S *pstUserTree, IN UINT64 uiOrgID)
{
    if (uiOrgID == 0)
    {
        NULL;
    }

    return NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
}

static UINT64 usertree_AddRootOrg(IN USER_TREE_S *pstUserTree, IN UINT64 uiOrgID)
{
    USER_TREE_ORG_S *pstOrg;

    pstOrg = NAP_ZAllocByID(pstUserTree->hOrgNap, uiOrgID);
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

    DLL_INIT(&pstOrg->stChildOrgList);
    DLL_INIT(&pstOrg->stChildUserList);

    return NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrg);
}

static VOID usertree_DeleteUser(IN USER_TREE_S *pstUserTree, IN USER_TREE_USER_S *pstUser)
{
    DLL_DEL(&pstUser->pstParentOrg->stChildUserList, pstUser);
    KV_Destory(pstUser->hKvList);
    NAP_Free(pstUserTree->hUserNap, pstUser);
}

static VOID usertree_DeleteOrg(IN USER_TREE_S *pstUserTree, IN USER_TREE_ORG_S *pstOrg)
{
    USER_TREE_ORG_S *pstOrgChild;
    USER_TREE_ORG_S *pstOrgTmp;
    USER_TREE_USER_S *pstUserChild;
    USER_TREE_USER_S *pstUserTmp;
    USER_TREE_ORG_S *pstParentOrg;

    
    DLL_SAFE_SCAN(&pstOrg->stChildOrgList, pstOrgChild, pstOrgTmp)
    {
        usertree_DeleteOrg(pstUserTree, pstOrgChild);
    }

    
    DLL_SAFE_SCAN(&pstOrg->stChildUserList, pstUserChild, pstUserTmp)
    {
        usertree_DeleteUser(pstUserTree, pstUserChild);
    }

    pstParentOrg = pstOrg->pParentOrg;
    if (NULL != pstParentOrg)
    {
        DLL_DEL(&pstParentOrg->stChildOrgList, pstOrg);
    }

    KV_Destory(pstOrg->hKvList);
    NAP_Free(pstUserTree->hOrgNap, pstOrg);
}

static VOID usertree_AddChildOrg
(
    IN USER_TREE_S *pstUserTree,
    IN USER_TREE_ORG_S *pstOrg,
    IN USER_TREE_ORG_S *pstChildOrg
)
{
    UINT64 uiChildOrgID = NAP_GetIDByNode(&pstUserTree->hOrgNap, pstChildOrg);
    UINT64 uiIdTmp;
    USER_TREE_ORG_S *pstOrgTmp;
    USER_TREE_ORG_S *pstOrgFind = NULL;

    DLL_SCAN(&pstOrg->stChildOrgList, pstOrgTmp)
    {
        uiIdTmp = NAP_GetIDByNode(&pstUserTree->hOrgNap, pstOrgTmp);
        if (uiIdTmp > uiChildOrgID)
        {
            pstOrgFind = pstOrgTmp;
            break;
        }
    }

    DLL_INSERT_BEFORE(&pstOrg->stChildOrgList, pstChildOrg, pstOrgFind);
}

static VOID usertree_AddChildUser
(
    IN USER_TREE_S *pstUserTree,
    IN USER_TREE_ORG_S *pstOrg,
    IN USER_TREE_USER_S *pstChildUser
)
{
    UINT64 uiChildID = NAP_GetIDByNode(&pstUserTree->hUserNap, pstChildUser);
    UINT64 uiIdTmp;
    USER_TREE_USER_S *pstTmp;
    USER_TREE_USER_S *pstFind = NULL;

    DLL_SCAN(&pstOrg->stChildUserList, pstTmp)
    {
        uiIdTmp = NAP_GetIDByNode(&pstUserTree->hUserNap, pstTmp);
        if (uiIdTmp > uiChildID)
        {
            pstFind = pstTmp;
            break;
        }
    }

    DLL_INSERT_BEFORE(&pstOrg->stChildUserList, pstChildUser, pstFind);
}

static USER_TREE_S * usertree_Create()
{
    USER_TREE_S *pstUserTree;

    pstUserTree = MEM_ZMalloc(sizeof(USER_TREE_S));
    if (NULL == pstUserTree)
    {
        return NULL;
    }

    NAP_PARAM_S param = {0};
    param.enType = NAP_TYPE_HASH;
    param.uiNodeSize = sizeof(USER_TREE_ORG_S);

    pstUserTree->hOrgNap = NAP_Create(&param);
    if (NULL == pstUserTree->hOrgNap)
    {
        UserTree_Destroy(pstUserTree);
        return NULL;
    }

    param.uiNodeSize = sizeof(USER_TREE_USER_S);
    pstUserTree->hUserNap = NAP_Create(&param);
    if (NULL == pstUserTree->hUserNap)
    {
        UserTree_Destroy(pstUserTree);
        return NULL;
    }

    return pstUserTree;
}

USER_TREE_HANDLE UserTree_Create()
{
    USER_TREE_S *pstUserTree;
    
    pstUserTree = usertree_Create();
    
    pstUserTree->uiRootOrgID = usertree_AddRootOrg(pstUserTree, 0);
    if (0 == pstUserTree->uiRootOrgID)
    {
        UserTree_Destroy(pstUserTree);
        return NULL;
    }

    return pstUserTree;
}

VOID UserTree_Destroy(IN USER_TREE_HANDLE hUserTree)
{
    USER_TREE_S *pstUserTree = hUserTree;

    if (pstUserTree->uiRootOrgID != 0)
    {
        UserTree_DeleteOrg(hUserTree, pstUserTree->uiRootOrgID);
    }

    if (NULL != pstUserTree->hUserNap)
    {
        NAP_Destory(pstUserTree->hUserNap);
    }

    if (NULL != pstUserTree->hOrgNap)
    {
        NAP_Destory(pstUserTree->hOrgNap);
    }

    MEM_Free(pstUserTree);
}

UINT64 UserTree_GetRootOrg(IN USER_TREE_HANDLE hUserTree)
{
    USER_TREE_S *pstUserTree = hUserTree;

    return pstUserTree->uiRootOrgID;
}

static UINT64 usertree_AddOrg(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcOrgName, IN UINT64 uiID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;
    USER_TREE_ORG_S *pstParentOrg;

    BS_DBGASSERT(uiParentOrgID != 0);

    pstParentOrg = usertree_GetOrgByID(pstUserTree, uiParentOrgID);
    if (NULL == pstParentOrg)
    {
        return 0;
    }

    pstOrg = NAP_ZAllocByID(pstUserTree->hOrgNap, uiID);
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
    DLL_INIT(&pstOrg->stChildOrgList);
    DLL_INIT(&pstOrg->stChildUserList);

    if (NULL != pstParentOrg)
    {
        usertree_AddChildOrg(pstUserTree, pstParentOrg, pstOrg);
        pstOrg->pParentOrg = pstParentOrg;
    }

    return NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrg);
}

UINT64 UserTree_AddOrg(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcOrgName)
{
    return usertree_AddOrg(hUserTree, uiParentOrgID, pcOrgName, 0);
}

static UINT64 usertree_AddUser(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcUserName, IN UINT64 uiID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_USER_S *pstUser;

    USER_TREE_ORG_S *pstParentOrg;

    BS_DBGASSERT(uiParentOrgID != 0);

    pstParentOrg = usertree_GetOrgByID(pstUserTree, uiParentOrgID);
    if (NULL == pstParentOrg)
    {
        return 0;
    }
 
    pstUser = NAP_ZAllocByID(pstUserTree->hUserNap, uiID);
    if (NULL == pstUser)
    {
        return 0;
    }

    pstUser->hKvList = KV_Create(0);
    if (NULL == pstUser->hKvList)
    {
        NAP_Free(pstUserTree->hUserNap, pstUser);
        return 0;
    }

    TXT_Strlcpy(pstUser->szUserName, pcUserName, sizeof(pstUser->szUserName));

    if (NULL != pstParentOrg)
    {
        usertree_AddChildUser(pstUserTree, pstParentOrg, pstUser);
        pstUser->pstParentOrg = pstParentOrg;
    }

    return NAP_GetIDByNode(pstUserTree->hUserNap, pstUser);
}

UINT64 UserTree_AddUser(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcUserName)
{
    return usertree_AddUser(hUserTree, uiParentOrgID, pcUserName, 0);
}

BS_STATUS UserTree_DeleteOrg(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;

    if (uiOrgID == 0)
    {
        return BS_BAD_PARA;
    }

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return BS_OK;
    }

    usertree_DeleteOrg(pstUserTree, pstOrg);

    return BS_OK;
}

BS_STATUS UserTree_DeleteUser(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_USER_S *pstUser;

    pstUser = NAP_GetNodeByID(pstUserTree->hUserNap, uiUserID);
    if (NULL == pstUser)
    {
        return BS_OK;
    }

    usertree_DeleteUser(pstUserTree, pstUser);

    return BS_OK;
}

CHAR * UserTree_GetOrgName(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return NULL;
    }

    return pstOrg->szOrgName;
}

CHAR * UserTree_GetUserName(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_USER_S *pstUser;

    pstUser = NAP_GetNodeByID(pstUserTree->hUserNap, uiUserID);
    if (NULL == pstUser)
    {
        return NULL;
    }

    return pstUser->szUserName;
}

BS_STATUS UserTree_SetOrgParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID, IN UINT64 uiParentOrgID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;
    USER_TREE_ORG_S *pstParentOrg;
    USER_TREE_ORG_S *pstParentOrgOld;

    if (uiOrgID == pstUserTree->uiRootOrgID)
    {
        return BS_NOT_SUPPORT;
    }

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return BS_NO_SUCH;
    }

    pstParentOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiParentOrgID);
    if (NULL == pstParentOrg)
    {
        return BS_NO_SUCH;
    }

    pstParentOrgOld = pstOrg->pParentOrg;
    if (pstParentOrg == pstParentOrgOld)
    {
        return BS_OK;
    }

    if (pstParentOrgOld != NULL)
    {
        DLL_DEL(&pstParentOrgOld->stChildOrgList, pstOrg);
    }

    usertree_AddChildOrg(pstUserTree, pstParentOrg, pstOrg);

    pstOrg->pParentOrg = pstParentOrg;

    return BS_OK;
}

UINT64 UserTree_GetOrgParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;

    if (uiOrgID == 0)
    {
        return 0;
    }

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    return NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrg->pParentOrg);
}

BS_STATUS UserTree_SetUserParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID, IN UINT64 uiParentOrgID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_USER_S *pstUser;
    USER_TREE_ORG_S *pstParentOrg;
    USER_TREE_ORG_S *pstParentOrgOld;

    pstUser = NAP_GetNodeByID(pstUserTree->hUserNap, uiUserID);
    if (NULL == pstUser)
    {
        return BS_NO_SUCH;
    }

    pstParentOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiParentOrgID);
    if (NULL == pstParentOrg)
    {
        return BS_NO_SUCH;
    }

    pstParentOrgOld = pstUser->pstParentOrg;
    if (pstParentOrg == pstParentOrgOld)
    {
        return BS_OK;
    }

    if (pstParentOrgOld != NULL)
    {
        DLL_DEL(&pstParentOrgOld->stChildUserList, pstUser);
    }

    usertree_AddChildUser(pstUserTree, pstParentOrg, pstUser);

    pstUser->pstParentOrg = pstParentOrg;

    return BS_OK;
}

UINT64 UserTree_GetUserParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_USER_S *pstUser;

    pstUser = NAP_GetNodeByID(pstUserTree->hUserNap, uiUserID);
    if (NULL == pstUser)
    {
        return 0;
    }

    return NAP_GetIDByNode(pstUserTree->hOrgNap, pstUser->pstParentOrg);
}

BS_STATUS UserTree_SetOrgKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcKey,
    IN CHAR *pcValue
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return BS_NO_SUCH;
    }

    return KV_SetKeyValue(pstOrg->hKvList, pcKey, pcValue);
}

BS_STATUS UserTree_SetUserKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiUserID,
    IN CHAR *pcKey,
    IN CHAR *pcValue
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_USER_S *pstUser;

    pstUser = NAP_GetNodeByID(pstUserTree->hUserNap, uiUserID);
    if (NULL == pstUser)
    {
        return BS_NO_SUCH;
    }

    return KV_SetKeyValue(pstUser->hKvList, pcKey, pcValue);
}

CHAR * UserTree_GetOrgKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcKey
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return NULL;
    }

    return KV_GetKeyValue(pstOrg->hKvList, pcKey);
}

CHAR * UserTree_GetUserKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiUserID,
    IN CHAR *pcKey
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_USER_S *pstUser;

    pstUser = NAP_GetNodeByID(pstUserTree->hUserNap, uiUserID);
    if (NULL == pstUser)
    {
        return NULL;
    }

    return KV_GetKeyValue(pstUser->hKvList, pcKey);
}

UINT64 UserTree_GetNextChildOrg
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN UINT64 uiCurrentChildOrgID
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;
    USER_TREE_ORG_S *pstOrgTmp;
    UINT64 uiTmpID;
    UINT64 uiNextChildID = 0;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    DLL_SCAN(&pstOrg->stChildOrgList, pstOrgTmp)
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

UINT64 UserTree_GetNextChildUser
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN UINT64 uiCurrentChildUserID
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;
    USER_TREE_USER_S *pstTmp;
    UINT64 uiTmpID;
    UINT64 uiNextChildID = 0;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    DLL_SCAN(&pstOrg->stChildUserList, pstTmp)
    {
        uiTmpID = NAP_GetIDByNode(pstUserTree->hOrgNap, pstTmp);
        if (uiTmpID > uiCurrentChildUserID)
        {
            uiNextChildID = uiTmpID;
            break;
        }
    }

    return uiNextChildID;
}

UINT64 UserTree_FindChildOrgByName
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcOrgName
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    UINT64 uiFoundID = 0;
    USER_TREE_ORG_S *pstOrg;
    USER_TREE_ORG_S *pstOrgTmp;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    DLL_SCAN(&pstOrg->stChildOrgList, pstOrgTmp)
    {
        if (strcmp(pstOrgTmp->szOrgName, pcOrgName) == 0)
        {
            uiFoundID = NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrgTmp);
            break;
        }
    }

    return uiFoundID;
}

UINT64 UserTree_FindChildUserByName
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcUserName
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    UINT64 uiFoundID = 0;
    USER_TREE_ORG_S *pstOrg;
    USER_TREE_USER_S *pstTmp;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, uiOrgID);
    if (NULL == pstOrg)
    {
        return 0;
    }

    DLL_SCAN(&pstOrg->stChildUserList, pstTmp)
    {
        if (strcmp(pstTmp->szUserName, pcUserName) == 0)
        {
            uiFoundID = NAP_GetIDByNode(pstUserTree->hUserNap, pstTmp);
            break;
        }
    }

    return uiFoundID;
}

UINT64 UserTree_FindUserByName
(
    IN USER_TREE_HANDLE hUserTree,
    IN CHAR *pcUserName
)
{
    UINT64 uiUserID = 0;
    USER_TREE_USER_S *pstTmp;
    USER_TREE_S *pstUserTree = hUserTree;

    while ((uiUserID = UserTree_GetNextUserByIndexOrder(hUserTree, uiUserID)) != 0)
    {
        pstTmp = NAP_GetNodeByID(pstUserTree->hUserNap, uiUserID);
        if ((NULL != pstTmp) && (strcmp(pstTmp->szUserName, pcUserName) == 0))
        {
            return uiUserID;
        }
    }

    return 0;
}


UINT64 UserTree_GetNextOrgByIndexOrder(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiCurrentOrgID)
{
    USER_TREE_S *pstUserTree = hUserTree;

    return NAP_GetNextID(pstUserTree->hOrgNap, uiCurrentOrgID);
}

UINT64 UserTree_GetNextUserByIndexOrder(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiCurrentUserID)
{
    USER_TREE_S *pstUserTree = hUserTree;

    return NAP_GetNextID(pstUserTree->hUserNap, uiCurrentUserID);
}


VOID UserTree_WalkUserByIndexOrder
(
    IN USER_TREE_HANDLE hUserTree,
    IN PF_UserTree_Walk_Func pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    UINT64 uiUserID = 0;
    int ret;

    while ((uiUserID = UserTree_GetNextUserByIndexOrder(hUserTree, uiUserID)) != 0) {
        ret = pfWalkFunc(hUserTree, uiUserID, hUserHandle);
        if (ret < 0) {
            break;
        }
    }
}

static int usertree_WalkOrgByDeepOrder
(
    IN USER_TREE_S *pstUserTree,
    IN USER_TREE_ORG_S *pstOrg,
    IN PF_UserTree_Walk_Func pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    USER_TREE_ORG_S *pstOrg1;
    USER_TREE_ORG_S *pstOrg2;
    int ret;
    
    ret = pfWalkFunc(pstUserTree, NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrg), hUserHandle);
    if (ret < 0) {
        return ret;
    }

    DLL_SAFE_SCAN(&pstOrg->stChildOrgList, pstOrg1, pstOrg2) {
        ret = usertree_WalkOrgByDeepOrder(pstUserTree, pstOrg1, pfWalkFunc, hUserHandle);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}


VOID UserTree_WalkOrgByDeepOrder
(
    IN USER_TREE_HANDLE hUserTree,
    IN PF_UserTree_Walk_Func pfWalkFunc,
    IN HANDLE hUserHandle
)
{
    USER_TREE_S *pstUserTree = hUserTree;
    USER_TREE_ORG_S *pstOrg;

    pstOrg = NAP_GetNodeByID(pstUserTree->hOrgNap, pstUserTree->uiRootOrgID);
    if (NULL == pstOrg)
    {
        return;
    }

    usertree_WalkOrgByDeepOrder(pstUserTree, pstOrg, pfWalkFunc, hUserHandle);
}

static int usertree_SequenceWalkOrg(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiID, IN HANDLE hUserHandle)
{
    USER_TREE_S *pstUserTree = hUserTree;
    cJSON *pstOrgJson = hUserHandle;
    cJSON *pstItem;
    USER_TREE_ORG_S *pstOrg;
    KV_S *pstKv = NULL;
    UINT64 uiParentID = 0;
    CHAR szID[32];

    pstOrg = usertree_GetOrgByID(hUserTree, uiID);

    if (NULL != pstOrg->pParentOrg)
    {
        uiParentID = NAP_GetIDByNode(pstUserTree->hOrgNap, pstOrg->pParentOrg);
    }

    snprintf(szID, sizeof(szID), "%llx", uiID);

    pstItem = cJSON_CreateObject();
    cJSON_AddStringToObject(pstItem, "_OrgName", pstOrg->szOrgName);
    snprintf(szID, sizeof(szID), "%llx", uiID);
    cJSON_AddStringToObject(pstItem, "_ID", szID);
    snprintf(szID, sizeof(szID),"%llx", uiParentID);
    cJSON_AddStringToObject(pstItem, "_ParentID", szID);

    while ((pstKv = KV_GetNext(pstOrg->hKvList, pstKv)) != NULL)
    {
        cJSON_AddStringToObject(pstItem, pstKv->pcKey, pstKv->pcValue);
    }

    cJSON_AddItemToArray(pstOrgJson, pstItem);

    return 0;
}

static int usertree_SequenceWalkUser(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiID, IN HANDLE hUserHandle)
{
    USER_TREE_S *pstUserTree = hUserTree;    
    USER_TREE_USER_S *pstUser;
    KV_S *pstKv = NULL;
    UINT64 uiParentID = 0;
    cJSON *pstUserJson = hUserHandle;
    cJSON *pstItem;
    CHAR szID[32];

    pstUser = NAP_GetNodeByID(pstUserTree->hUserNap, uiID);

    if (NULL != pstUser->pstParentOrg)
    {
        uiParentID = NAP_GetIDByNode(pstUserTree->hOrgNap, pstUser->pstParentOrg);
    }

    pstItem = cJSON_CreateObject();
    cJSON_AddStringToObject(pstItem, "_UserName", pstUser->szUserName);
    snprintf(szID, sizeof(szID),"%llx", uiID);
    cJSON_AddStringToObject(pstItem, "_ID", szID);
    snprintf(szID, sizeof(szID),"%llx", uiParentID);
    cJSON_AddStringToObject(pstItem, "_ParentID", szID);

    while ((pstKv = KV_GetNext(pstUser->hKvList, pstKv)) != NULL)
    {
        cJSON_AddStringToObject(pstItem, pstKv->pcKey, pstKv->pcValue);
    }

    cJSON_AddItemToArray(pstUserJson, pstItem);

    return 0;
}


CHAR * UserTree_Sequence(IN USER_TREE_HANDLE hUserTree)
{
    cJSON *pstJson;
    cJSON *pstOrg;
    cJSON *pstUser;
    CHAR *pcInfo;

    pstJson = cJSON_CreateObject();
    if (NULL == pstJson)
    {
        return NULL;
    }

    pstOrg = cJSON_CreateArray();
    pstUser = cJSON_CreateArray();

    UserTree_WalkOrgByDeepOrder(hUserTree, usertree_SequenceWalkOrg, pstOrg);
    UserTree_WalkUserByIndexOrder(hUserTree, usertree_SequenceWalkUser, pstUser);

    cJSON_AddItemToObject(pstJson, "Org", pstOrg);
    cJSON_AddItemToObject(pstJson, "User", pstUser);

    pcInfo = cJSON_Print(pstJson);

    cJSON_Delete(pstJson);

    return pcInfo;
}

static BS_STATUS usertree_RestoreOrg(IN USER_TREE_S *pstUserTree, IN cJSON *pstOrg)
{
    cJSON *pstName;
    cJSON *pstID;
    cJSON *pstParent;
    UINT64 uiID = 0;
    UINT64 uiParentID;
    cJSON *pstC;

    pstName = cJSON_GetObjectItem(pstOrg, "_OrgName");
    pstID = cJSON_GetObjectItem(pstOrg, "_ID");
    pstParent = cJSON_GetObjectItem(pstOrg, "_ParentID");

    sscanf(pstID->valuestring, "%llx", &uiID);
    sscanf(pstParent->valuestring, "%llx", &uiParentID);

    if (uiParentID == 0)
    {
        uiID = usertree_AddRootOrg(pstUserTree, uiID);
        pstUserTree->uiRootOrgID = uiID;
    }
    else
    {
        uiID = usertree_AddOrg(pstUserTree, uiParentID, pstName->valuestring, uiID);
    }

    if (uiID == 0)
    {
        return BS_ERR;
    }

    for (pstC = pstOrg->child; pstC != NULL; pstC=pstC->next)
    {
        if (pstC->string[0] == '_')
        {
            continue;
        }

        UserTree_SetOrgKeyValue(pstUserTree, uiID, pstC->string, pstC->valuestring);
    }

    return BS_OK;
}

static BS_STATUS usertree_RestoreUser(IN USER_TREE_S *pstUserTree, IN cJSON *pstUser)
{
    cJSON *pstName;
    cJSON *pstID;
    cJSON *pstParent;
    UINT64 uiID = 0;
    UINT64 uiParentID;
    cJSON *pstC;

    pstName = cJSON_GetObjectItem(pstUser, "_UserName");
    pstID = cJSON_GetObjectItem(pstUser, "_ID");
    pstParent = cJSON_GetObjectItem(pstUser, "_ParentID");

    sscanf(pstID->valuestring, "%llx", &uiID);
    sscanf(pstParent->valuestring, "%llx", &uiParentID);

    uiID = usertree_AddUser(pstUserTree, uiParentID, pstName->valuestring, uiID);
    if (0 == uiID)
    {
        return BS_ERR;
    }

    for (pstC = pstUser->child; pstC != NULL; pstC=pstC->next)
    {
        if (pstC->string[0] == '_')
        {
            continue;
        }

        UserTree_SetUserKeyValue(pstUserTree, uiID, pstC->string, pstC->valuestring);
    }

    return BS_OK;
}


USER_TREE_HANDLE UserTree_DeSequence(IN CHAR *pcInfo)
{
    cJSON * pstJson;
    cJSON *pstOrgs;
    cJSON *pstUsers;
    cJSON *pstOrg;
    cJSON *pstUser;
    int i;
    USER_TREE_S *pstUserTree;

    pstJson = cJSON_Parse(pcInfo);
    if (NULL == pstJson)
    {
        return NULL;
    }

    pstUserTree = usertree_Create();
    if (NULL == pstUserTree)
    {
        cJSON_Delete(pstJson);
        return NULL;
    }

    pstOrgs = cJSON_GetObjectItem(pstJson, "Org");
    pstUsers = cJSON_GetObjectItem(pstJson, "User");

    i = 0;
    while ((pstOrg = cJSON_GetArrayItem(pstOrgs, i++)) != NULL)
    {
        usertree_RestoreOrg(pstUserTree, pstOrg);
    }

    i = 0;
    while ((pstUser = cJSON_GetArrayItem(pstUsers, i++)) != NULL)
    {
        usertree_RestoreUser(pstUserTree, pstUser);
    }

    cJSON_Delete(pstJson);

    return pstUserTree;
}

