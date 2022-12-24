/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-12-6
* Description: 
* History:     
******************************************************************************/

#ifndef __CDOMAIN_UTL_H_
#define __CDOMAIN_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "utl/dc_utl.h"
#include "utl/ulm_utl.h"

#define CDOMAIN_MAX_CDOMAIN_NEM_LEN 63
#define CDOMAIN_MAX_USER_NAME_LEN  63
#define CDOMAIN_MAX_USER_GROUP_NAME_LEN 63

typedef enum
{
    CDOMAIN_USER_LOGIN_RESULT_OK = 0,
    CDOMAIN_USER_LOGIN_RESULT_FAIL,
    CDOMAIN_USER_LOGIN_RESULT_BLACKLIST
}CDOMAIN_USER_LOGIN_RESULT_E;

/* 创建域管理实例 */
/*输入：
ulMaxDomainNum：可创建的最大域个数
eDcType：域管理实例的数据容器类型
pParam：数据容器需要的参数.
*/
BS_STATUS CDOMAIN_OpenInstance
(
    IN UINT ulMaxDomainNum,
    IN DC_TYPE_E eDcType,
    IN VOID *pParam,
    OUT HANDLE *phInstance
);

BS_STATUS CDOMAIN_CloseInstance(IN HANDLE hInstance);

/* 创建域*/
/*输入:
ulInstanceId:域管理实例ID
szDomainName: 域名字
*/
BS_STATUS CDOMAIN_CreateDomain(IN HANDLE hInstance, IN CHAR szDomainName[CDOMAIN_MAX_CDOMAIN_NEM_LEN + 1]);

/* 删除域*/
BS_STATUS CDOMAIN_DelDomain(IN HANDLE hInstance, IN CHAR * pszDomainName);

BS_STATUS CDOMAIN_SetMaxUserNum(IN HANDLE hInstance, IN CHAR * pszDomainName, IN UINT ulMaxUserNum);

UINT CDOMAIN_GetMaxUserNum(IN HANDLE hInstance, IN CHAR * pszDomainName);

/* 向域挂接数据*/
BS_STATUS CDOMAIN_SetUserHandleToDomain
(
    IN HANDLE hInstance,
    IN CHAR *pszDomainName,
    IN CHAR *pszKey,
    IN CHAR *pszValue
);

/* 取得域挂接的数据*/
BS_STATUS CDOMAIN_GetUserHandleFromDomain
(
    IN HANDLE hInstance,
    IN CHAR *pszDomainName,
    IN CHAR *pszKey,
    OUT CHAR *pszValue,
    IN UINT ulValueLen  /* 传入可以拷贝的最大字符串长度,包含最后的'\0 */
);

/* 回调函数定义*/
typedef BS_WALK_RET_E (*PF_CDOMAIN_WALK_CDOMAIN_FUNC)(IN HANDLE hInstance, IN CHAR * pszDomainName, IN HANDLE hUserHandle);

/* 遍历函数 */
VOID CDOMAIN_WalkDomain
(
    IN HANDLE hInstance,
    IN PF_CDOMAIN_WALK_CDOMAIN_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
);

/*  添加超级管理员 */
BS_STATUS CDOMAIN_AddSuperAdmin(IN HANDLE hInstance, IN CHAR *pszUserName, IN CHAR *pszPasswd);

/* 删除超级管理员 */
BS_STATUS CDOMAIN_DelSuperAdmin(IN HANDLE hInstance, IN CHAR *pszUserName);


/* 添加域管理员 */
BS_STATUS CDOMAIN_AddAdmin(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName, IN CHAR *pszPasswd);

/* 删除Admin */
BS_STATUS CDOMAIN_DelAdmin(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName);

/* 添加用户 */
BS_STATUS CDOMAIN_AddUser(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName, IN CHAR *pszPasswd);

/* 删除用户 */
BS_STATUS CDOMAIN_DelUser(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName);

/* 得到域中超级管理员数目 */
UINT CDOMAIN_GetSAdminNum(IN HANDLE hInstance);

/* 得到域中管理员数目 */
UINT CDOMAIN_GetAdminNum(IN HANDLE hInstance, IN CHAR * pszDomainName);

/* 得到域中用户数目 */
UINT CDOMAIN_GetUserNum(IN HANDLE hInstance, IN CHAR * pszDomainName);

/* 将用户加入黑名单 */
BS_STATUS CDOMAIN_AddUserToBlackList(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName);

/*  将用户从黑名单中删除 */
BS_STATUS CDOMAIN_DelUserFromBlackList(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName);

/* 设置用户只读属性 */
BS_STATUS CDOMAIN_SetUserReadOnly
(
    IN HANDLE hInstance,
    IN CHAR *pszDomainName,
    IN CHAR *pszUserName,
    IN BOOL_T bReadOnly
);

BS_STATUS CDOMAIN_GetUserReadOnlyAttribute
(
    IN HANDLE hInstance,
    IN CHAR *pszDomainName,
    IN CHAR *pszUserName,
    OUT BOOL_T *pbReadOnly
);

/*  向用户挂接数据 */
BS_STATUS CDOMAIN_SetUserHandleToUser(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName, IN CHAR *pszKey, IN CHAR *pszValue);

/* 取得用户挂接的数据 */
BS_STATUS CDOMAIN_GetUserHandleFromUser
(
    IN HANDLE hInstance,
    IN CHAR * pszDomainName,
    IN CHAR *pszUserName,
    IN CHAR *pszKey,
    OUT CHAR *pszValue,
    IN UINT ulValueLen  /* 传入可以拷贝的最大字符串长度,包含最后的'\0 */
);

/*  遍历域中所有用户 */
/* 回调函数定义 */
typedef BS_WALK_RET_E (*PF_CDOMAIN_WALK_USER_IN_CDOMAIN_FUNC)
    (IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName, IN HANDLE hUserHandle);

/* 遍历函数 */
VOID CDOMAIN_WalkUserInDomain
(
    IN HANDLE hInstance,
    IN CHAR * pszDomainName,
    IN PF_CDOMAIN_WALK_USER_IN_CDOMAIN_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
);

VOID CDOMAIN_WalkAdminInDomain
(
    IN HANDLE hInstance,
    IN CHAR * pszDomainName,
    IN PF_CDOMAIN_WALK_USER_IN_CDOMAIN_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
);

VOID CDOMAIN_WalkSAdminInDomain
(
    IN HANDLE hInstance,
    IN PF_CDOMAIN_WALK_USER_IN_CDOMAIN_FUNC pfWalkFunc,
    IN HANDLE hUserHandle
);

/*  遍历用户所属的组 */
/* 回调函数定义 */
typedef BS_WALK_RET_E (*PF_CDOMAIN_WALK_GROUP_OF_USER_FUNC)
    (IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName, IN CHAR *pszUerGroupName, IN HANDLE hUserHandle);



/*  注册用户事件 */
/* 事件响应函数类型 */
typedef VOID (*PF_CDOMAIN_USER_EVENT_FUNC)
    (IN HANDLE hInstance, IN CHAR *pszDomainName, IN CHAR *pszUserName, IN UINT ulEvent, IN USER_HANDLE_S *pstUserHandle);

/* 创建用户组 */
BS_STATUS CDOMAIN_CreateUserGroup(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserGroupName);


/* 删除用户组 */
BS_STATUS CDOMAIN_DelUserGroup(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserGroupName);

/* 将用户和用户组关联 */
BS_STATUS CDOMAIN_AddUserToGroup(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName, IN CHAR *pszUserGroupName);

/*  将用户从用户组中解关联 */
BS_STATUS CDOMAIN_DelUserFromGroup(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserName, IN CHAR *pszUserGroupName);

/*  向用户组挂接数据 */
BS_STATUS CDOMAIN_SetUserHandleToUserGroup(IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserGroupName, IN CHAR *pszKey, IN CHAR *pszValue);

/*  取得用户组挂接的数据 */
BS_STATUS CDOMAIN_GetUserHandleFromUserGroup
(
    IN HANDLE hInstance,
    IN CHAR * pszDomainName,
    IN CHAR *pszUserGroupName,
    IN CHAR *pszKey,
    OUT CHAR *pszValue,
    IN UINT ulValueLen
);

/*  遍历用户组中的用户 */
/* 回调函数定义 */
typedef VOID (*PF_CDOMAIN_WALK_USER_IN_GROUP_FUNC)
    (IN HANDLE hInstance, IN CHAR * pszDomainName, IN CHAR *pszUserGroupName, IN CHAR *pszUserName, IN UINT ulUserHandle);

/*  超级管理员登陆 */
/* 返回值：用户ID。 如果为0，则表示登录失败。 */
CDOMAIN_USER_LOGIN_RESULT_E CDOMAIN_CheckSuperAdmin(IN HANDLE hInstance, IN CHAR *pszUserName, IN CHAR *pszPassWd);

/*  域管理员登陆 */
CDOMAIN_USER_LOGIN_RESULT_E CDOMAIN_CheckAdmin(IN HANDLE hInstance, IN CHAR *pszDomainName, IN CHAR *pszUserName, IN CHAR *pszPassWd);

/*  用户登录 */
CDOMAIN_USER_LOGIN_RESULT_E CDOMAIN_CheckUser(IN HANDLE hInstance, IN CHAR *pszDomainName, IN CHAR *pszUserName, IN CHAR *pszPassWd);

BS_STATUS CDOMAIN_GetUserPropertyAsUint
(
    IN HANDLE hInstance,
    IN CHAR *pszDomainName,
    IN CHAR *pszUserName,
    IN CHAR *pcPropertyName,
    OUT UINT *puiValue
);

BS_STATUS CDOMAIN_Save(IN HANDLE hInstance);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__CDOMAIN_UTL_H_*/


