/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-4-21
* Description: 
* History:     
******************************************************************************/

#ifndef __USER_TREE_H_
#define __USER_TREE_H_

#include "utl/string_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE USER_TREE_HANDLE;

typedef BS_WALK_RET_E (*PF_UserTree_Walk_Func)(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiID, IN HANDLE hUserHandle);

USER_TREE_HANDLE UserTree_Create();
VOID UserTree_Destroy(IN USER_TREE_HANDLE hUserTree);
UINT64 UserTree_GetRootOrg(IN USER_TREE_HANDLE hUserTree);
UINT64 UserTree_AddOrg(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcOrgName);
UINT64 UserTree_AddUser(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiParentOrgID, IN CHAR *pcUserName);
BS_STATUS UserTree_DeleteOrg(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID);
BS_STATUS UserTree_DeleteUser(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID);
CHAR * UserTree_GetOrgName(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID);
CHAR * UserTree_GetUserName(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID);
BS_STATUS UserTree_SetOrgParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID, IN UINT64 uiParentOrgID);
UINT64 UserTree_GetOrgParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiOrgID);
BS_STATUS UserTree_SetUserParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID, IN UINT64 uiParentOrgID);
UINT64 UserTree_GetUserParent(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiUserID);
BS_STATUS UserTree_SetOrgKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcKey,
    IN CHAR *pcValue
);
BS_STATUS UserTree_SetUserKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiUserID,
    IN CHAR *pcKey,
    IN CHAR *pcValue
);
CHAR * UserTree_GetOrgKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcKey
);
CHAR * UserTree_GetUserKeyValue
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiUserID,
    IN CHAR *pcKey
);
UINT64 UserTree_GetNextChildOrg
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN UINT64 uiCurrentChildOrgID
);
UINT64 UserTree_GetNextChildUser
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN UINT64 uiCurrentChildUserID
);
UINT64 UserTree_FindChildOrgByName
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcOrgName
);
UINT64 UserTree_FindChildUserByName
(
    IN USER_TREE_HANDLE hUserTree,
    IN UINT64 uiOrgID,
    IN CHAR *pcUserName
);

UINT64 UserTree_FindUserByName
(
    IN USER_TREE_HANDLE hUserTree,
    IN CHAR *pcUserName
);

/* 根据Index顺序获取下一个 */
UINT64 UserTree_GetNextOrgByIndexOrder(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiCurrentOrgID);
UINT64 UserTree_GetNextUserByIndexOrder(IN USER_TREE_HANDLE hUserTree, IN UINT64 uiCurrentUserID);

/* 序列化 */
CHAR * UserTree_Sequence(IN USER_TREE_HANDLE hUserTree);
/* 反序列化 */
USER_TREE_HANDLE UserTree_DeSequence(IN CHAR *pcInfo);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__USER_TREE_H_*/


