/******************************************************************************
* Copyright (C), LiXingang
* Author:      lixingang  Version: 1.0  Date: 2008-2-1
* Description: User Online Managment
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_ULM

#include "bs.h"

#include "utl/data2hex_utl.h"
#include "utl/txt_utl.h"
#include "utl/nap_utl.h"
#include "utl/time_utl.h"
#include "utl/kv_utl.h"
#include "utl/ulm_utl.h"

#define _ULM_INVALID_INDEX_ID 0xffffffff

#define _ULM_GET_INDEX_BY_ULM_ID(ulUlmId)   ((ulUlmId) & 0xffff)
#define _ULM_GET_INCNUM_BY_ULM_ID(ulUlmId)  (((ulUlmId) >>16) & 0xffff)



typedef struct
{
	CHAR  szUserName[ULM_MAX_USER_NAME_LEN + 1];
	UCHAR szUserCookie[ULM_USER_COOKIE_LEN + 1];
    ULONG ulLoginTime;	
    BOOL_T bIsStopTimeOut;  
    UINT ulTimeTickLeft;  
    UINT ulRefNum;    
    UINT ulMaxRefNum; 
    UINT uiUserFlag;  
    HANDLE hUserHandle;
    KV_HANDLE hKvList;  
}_ULM_NODE_S;

typedef struct
{
    NAP_HANDLE hNap;
    UINT ulMaxUserNum;
    UINT ulTimeOutTime;    
    BOOL_T bIsTimeOutIgnorRef;   
    PF_ULM_USER_DEL_NOTIFY pfUserDelNotifyFunc;
}_ULM_INSTANCE_S;


static _ULM_INSTANCE_S * _ULM_CreateInstance(IN UINT ulMaxUserNum)
{
    _ULM_INSTANCE_S *pstUlmInstance;

    pstUlmInstance = MEM_ZMalloc(sizeof(_ULM_INSTANCE_S));
    if (NULL == pstUlmInstance)
    {
        return NULL;
    }

    NAP_PARAM_S param = {0};
    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = ulMaxUserNum;
    param.uiNodeSize = sizeof(_ULM_NODE_S);

    pstUlmInstance->hNap = NAP_Create(&param);
    if (NULL == pstUlmInstance->hNap)
    {
        MEM_Free(pstUlmInstance);
        return NULL;
    }

    pstUlmInstance->ulMaxUserNum = ulMaxUserNum;

    return pstUlmInstance;
}

static inline UINT _ULM_GetUserIdByName
(
    IN _ULM_INSTANCE_S *pstInstance,
    IN CHAR *pszUserName
 )
{
    UINT ulUserId = 0;
    _ULM_NODE_S *pstUser;

    while (0 != (ulUserId = (UINT)NAP_GetNextID(pstInstance->hNap, ulUserId)))
    {
        pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
        if (pstUser == NULL)
        {
            continue;
        }

        if (strcmp(pstUser->szUserName, pszUserName) == 0)
        {
            return ulUserId;
        }
    }

    return 0;    
}

static inline UINT _ULM_AddUser
(
    IN _ULM_INSTANCE_S *pstInstance,
    IN CHAR *pszUserName
)
{
    UINT ulUserId;
    _ULM_NODE_S *pstNode;

    
    ulUserId = _ULM_GetUserIdByName(pstInstance, pszUserName);
    if (ulUserId != 0)
    {
        return ulUserId;
    }

    pstNode = NAP_ZAlloc(pstInstance->hNap);
    if (NULL == pstNode)
    {
        return 0;
    }

    TXT_StrCpy(pstNode->szUserName, pszUserName);
    pstNode->ulLoginTime = (ULONG)TM_NowInSec();
    pstNode->ulTimeTickLeft = pstInstance->ulTimeOutTime;

    return (UINT) NAP_GetIDByNode(pstInstance->hNap, pstNode);
}

static inline BS_STATUS _ULM_DelUser(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    if (NULL != pstInstance->pfUserDelNotifyFunc)
    {
        
        pstInstance->pfUserDelNotifyFunc(pstInstance, ulUserId);
    }

    if (NULL != pstUser->hKvList)
    {
        KV_Destory(pstUser->hKvList);
        pstUser->hKvList = NULL;
    }

    NAP_Free(pstInstance->hNap, pstUser);

    return BS_OK;
}

static inline BS_STATUS _ULM_DelAllUser(IN _ULM_INSTANCE_S *pstInstance)
{
    UINT uiID = 0;

    while (0 != (uiID = (UINT)NAP_GetNextID(pstInstance->hNap, uiID)))
    {
        _ULM_DelUser(pstInstance, uiID);
    }

    return BS_OK;
}

static inline BS_STATUS _ULM_SetUserHandle(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId, IN HANDLE hUserHandle)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    pstUser->hUserHandle = hUserHandle;

    return BS_OK;
}

static inline BS_STATUS _ULM_GetUserHandle(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId, OUT HANDLE *phUserHandle)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    *phUserHandle = pstUser->hUserHandle;

    return BS_OK;
}

static inline BS_STATUS _ULM_SetUserKeyValue
(
    IN _ULM_INSTANCE_S *pstInstance,
    IN UINT ulUserId,
    IN CHAR *pcKey,
    IN CHAR *pcValue
)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    if (NULL == pstUser->hKvList)
    {
        pstUser->hKvList = KV_Create(0);
        if (NULL == pstUser->hKvList)
        {
            RETURN(BS_NO_MEMORY);
        }
    }

    return KV_SetKeyValue(pstUser->hKvList, pcKey, pcValue);
}

static inline CHAR * _ULM_GetUserKeyValue
(
    IN _ULM_INSTANCE_S *pstInstance,
    IN UINT ulUserId,
    IN CHAR *pcKey
)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        return NULL;
    }

    return KV_GetKeyValue(pstUser->hKvList, pcKey);
}

static inline BS_STATUS _ULM_SetUserFlag(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId, IN UINT uiUserFlag)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    pstUser->uiUserFlag = uiUserFlag;

    return BS_OK;
}

static inline BS_STATUS _ULM_GetUserFlag(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId, OUT UINT *puiUserFlag)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    *puiUserFlag = pstUser->uiUserFlag;

    return BS_OK;
}

static inline BS_STATUS _ULM_GetUserInfo(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId, OUT ULM_USER_INFO_S *pstUserInfo)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    TXT_StrCpy(pstUserInfo->szUserName, pstUser->szUserName);
    pstUserInfo->ulLoginTime = pstUser->ulLoginTime;

    return BS_OK;
}

static CHAR * _ULM_GetUserNameById(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        return NULL;
    }

    return pstUser->szUserName;
}

static inline BS_STATUS _ULM_SetTimeOutTime(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulTimeOutTime )
{
    pstInstance->ulTimeOutTime = ulTimeOutTime;

    return BS_OK;
}

static inline BS_STATUS _ULM_GetTimeOutTime(IN _ULM_INSTANCE_S *pstInstance, OUT UINT *pulTimeOutTime )
{
    *pulTimeOutTime = pstInstance->ulTimeOutTime;

    return BS_OK;
}

static BS_STATUS _ULM_TimeOut(IN _ULM_INSTANCE_S *pstInstance)
{
    _ULM_NODE_S *pstUser;
    UINT uiIndex = NAP_INVALID_INDEX;

    while (NAP_INVALID_INDEX !=
            (uiIndex = NAP_GetNextIndex(pstInstance->hNap, uiIndex))) {
        pstUser = NAP_GetNodeByIndex(pstInstance->hNap, uiIndex);
        if (pstUser->bIsStopTimeOut == TRUE) {
            continue;
        }

        if ((! pstInstance->bIsTimeOutIgnorRef) && (pstUser->ulRefNum != 0)) {
            continue;
        }

        if (pstUser->ulTimeTickLeft > 0) {
            pstUser->ulTimeTickLeft --;
        }

        if (pstUser->ulTimeTickLeft == 0) {
            _ULM_DelUser(pstInstance,
                    (UINT)NAP_GetIDByNode(pstInstance->hNap, pstUser));
        }
    }

    return BS_OK;
}

static inline BS_STATUS _ULM_StartTimeOut(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    if (pstUser->bIsStopTimeOut == TRUE)
    {
        pstUser->ulTimeTickLeft = pstInstance->ulTimeOutTime;
    }

    pstUser->bIsStopTimeOut = FALSE;

    return BS_OK;
}

static inline BS_STATUS _ULM_StopTimeOut(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    pstUser->bIsStopTimeOut = TRUE;

    return BS_OK;
}

static inline BS_STATUS _ULM_ResetTimeOut(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    pstUser->ulTimeTickLeft = pstInstance->ulTimeOutTime;

    return BS_OK;
}

static inline BS_STATUS _ULM_StopAllUserTimeOut(IN _ULM_INSTANCE_S *pstInstance)
{
    UINT uiID = 0;

    while (0 != (uiID = (UINT)NAP_GetNextID(pstInstance->hNap, uiID)))
    {
        _ULM_StopTimeOut(pstInstance, uiID);
    }

    return BS_OK;
}

static inline BS_STATUS _ULM_StartAllUserTimeOut(IN _ULM_INSTANCE_S *pstInstance)
{
    UINT uiID = 0;

    while (0 != (uiID = (UINT)NAP_GetNextID(pstInstance->hNap, uiID)))
    {
        _ULM_StartTimeOut(pstInstance, uiID);
    }

    return BS_OK;
}

static inline BS_STATUS _ULM_SetUserCookie(IN _ULM_INSTANCE_S *pstUlmInstance, IN UINT ulUserId, IN CHAR szCookie[ULM_USER_COOKIE_LEN+1])
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstUlmInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    TXT_StrCpy((CHAR*)pstUser->szUserCookie, szCookie);

    return BS_OK;
}

static inline CHAR * _ULM_GetUserCookie(IN _ULM_INSTANCE_S *pstUlmInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;
    UINT i;

    pstUser = NAP_GetNodeByID(pstUlmInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        return NULL;
    }

    if (pstUser->szUserCookie[0] == '\0')
    {
        DH_Data2Hex((UCHAR *)&ulUserId, sizeof(UINT), (CHAR*)pstUser->szUserCookie);

        
        for (i=sizeof(UINT) * 2; i<ULM_USER_COOKIE_LEN; i++)
        {
            pstUser->szUserCookie[i] = TXT_Random();
        }
        pstUser->szUserCookie[ULM_USER_COOKIE_LEN] = '\0';
    }

    return (CHAR*)pstUser->szUserCookie;
}

static inline BS_STATUS _ULM_SetMaxUserRefNum(IN _ULM_INSTANCE_S *pstUlmInstance, IN UINT ulUserId, IN UINT ulMaxRefNum)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstUlmInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    pstUser->ulMaxRefNum = ulMaxRefNum;

    return BS_OK;
}

static inline BS_STATUS _ULM_IncUserRefNum(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    if ((pstUser->ulMaxRefNum != 0) && (pstUser->ulMaxRefNum <= pstUser->ulRefNum))
    {
        RETURN(BS_FULL);
    }

    pstUser->ulRefNum++;

    return BS_OK;
}

static inline BS_STATUS _ULM_DecUserRefNum(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    pstUser->ulRefNum--;

    if (pstInstance->bIsTimeOutIgnorRef == FALSE)
    {
        pstUser->ulTimeTickLeft = pstInstance->ulTimeOutTime;
    }

    return BS_OK;
}

static inline BS_STATUS _ULM_GetUserRefNum(IN _ULM_INSTANCE_S *pstInstance, IN UINT ulUserId, OUT UINT *pulRefNum)
{
    _ULM_NODE_S *pstUser;

    pstUser = NAP_GetNodeByID(pstInstance->hNap, ulUserId);
    if (NULL == pstUser)
    {
        RETURN(BS_NO_SUCH);
    }

    *pulRefNum = pstUser->ulRefNum;

    return BS_OK;
}

static inline UINT _ULM_GetNextUser(IN _ULM_INSTANCE_S *pstInstance, IN UINT uiCurrentID)
{
    return (UINT)NAP_GetNextID(pstInstance->hNap, uiCurrentID);
}

ULM_HANDLE ULM_CreateInstance(IN UINT ulMaxUserNum )
{
    return _ULM_CreateInstance(ulMaxUserNum);
}

BS_STATUS ULM_DesTroyInstance(IN ULM_HANDLE hUlmHandle)
{
    _ULM_INSTANCE_S *pstUlmInstance = hUlmHandle;

    NAP_Destory(pstUlmInstance->hNap);
    MEM_Free(pstUlmInstance);

    return BS_OK;
}


UINT ULM_AddUser(IN ULM_HANDLE hUlmHandle, IN CHAR *pszUserName)
{
    BS_DBGASSERT(NULL != pszUserName);

    return _ULM_AddUser(hUlmHandle, pszUserName);
}

BS_STATUS ULM_DelUser(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId)
{
    return _ULM_DelUser(hUlmHandle, ulUserId);
}

BS_STATUS ULM_DelAllUser(IN ULM_HANDLE hUlmHandle)
{
    return _ULM_DelAllUser(hUlmHandle);
}

BS_STATUS ULM_SetUserHandle(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, IN HANDLE hUserHandle)
{
    return _ULM_SetUserHandle(hUlmHandle, ulUserId, hUserHandle);
}

BS_STATUS ULM_GetUserHandle(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, OUT HANDLE *phUserHandle)
{
    return _ULM_GetUserHandle(hUlmHandle, ulUserId, phUserHandle);
}

BS_STATUS ULM_SetUserKeyValue(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, IN CHAR *pcKey, IN CHAR *pcValue)
{
    return _ULM_SetUserKeyValue(hUlmHandle, ulUserId, pcKey, pcValue);
}

CHAR * ULM_GetUserKeyValue(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, IN CHAR *pcKey)
{
    return _ULM_GetUserKeyValue(hUlmHandle, ulUserId, pcKey);
}

BS_STATUS ULM_SetUserFlag(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, IN UINT uiFlag)
{
    return _ULM_SetUserFlag(hUlmHandle, ulUserId, uiFlag);
}

BS_STATUS ULM_GetUserFlag(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, OUT UINT *puiFlag)
{
    return _ULM_GetUserFlag(hUlmHandle, ulUserId, puiFlag);
}

BS_STATUS ULM_SetUserDelNotifyFunc(IN ULM_HANDLE hUlmHandle, IN PF_ULM_USER_DEL_NOTIFY pfFunc)
{
    _ULM_INSTANCE_S *pstInstance = hUlmHandle;
    
    pstInstance->pfUserDelNotifyFunc = pfFunc;

    return BS_OK;
}

BS_STATUS ULM_GetUserInfo(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, OUT ULM_USER_INFO_S *pstUserInfo)
{
    if (NULL == pstUserInfo)
    {
        RETURN(BS_NULL_PARA);
    }
    
    return _ULM_GetUserInfo(hUlmHandle, ulUserId, pstUserInfo);
}

BS_STATUS ULM_SetUserCookie(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, IN CHAR szCookie[ULM_USER_COOKIE_LEN+1])
{
    if (NULL == szCookie)
    {
        RETURN(BS_NULL_PARA);
    }

    return _ULM_SetUserCookie(hUlmHandle, ulUserId, szCookie);
}

CHAR * ULM_GetUserCookie(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId)
{
    return _ULM_GetUserCookie(hUlmHandle, ulUserId);
}

UINT ULM_GetUserIDByCookie(IN ULM_HANDLE hUlmHandle, IN CHAR* pcCookie)
{
    UINT uiUserID;
    CHAR *pcCookieSaved;

    if (NULL == pcCookie)
    {
        return 0;
    }

    DH_Hex2Data(pcCookie, sizeof(UINT)* 2, (UCHAR*)&uiUserID);

    pcCookieSaved = ULM_GetUserCookie(hUlmHandle, uiUserID);
    if (NULL == pcCookieSaved)
    {
        return 0;
    }

    if (strcmp(pcCookie, pcCookieSaved) != 0)
    {
        return 0;
    }

    return uiUserID;
}

UINT ULM_GetNextUserID(IN ULM_HANDLE hUlmHandle, IN UINT uiCurrentUserID)
{
    return _ULM_GetNextUser(hUlmHandle, uiCurrentUserID);
}

UINT ULM_GetUserIdByName(IN ULM_HANDLE hUlmHandle, IN CHAR *pszName)
{
    return _ULM_GetUserIdByName(hUlmHandle, pszName);
}

CHAR * ULM_GetUserNameById(IN ULM_HANDLE hUlmHandle, IN UINT uiUserID)
{
    return _ULM_GetUserNameById(hUlmHandle, uiUserID);
}

BS_STATUS ULM_SetMaxUserRefNum(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, IN UINT ulMaxRefNum)
{
    return _ULM_SetMaxUserRefNum(hUlmHandle, ulUserId, ulMaxRefNum);
}

BS_STATUS ULM_IncUserRefNum(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId)
{
    return _ULM_IncUserRefNum(hUlmHandle, ulUserId);
}

BS_STATUS ULM_DecUserRefNum(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId)
{
    return _ULM_DecUserRefNum(hUlmHandle, ulUserId);
}

BS_STATUS ULM_GetUserRefNum(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId, OUT UINT *pulRefNum)
{
    BS_DBGASSERT(NULL != pulRefNum);
    return _ULM_GetUserRefNum(hUlmHandle, ulUserId, pulRefNum);
}

BS_STATUS ULM_SetTimeOutTime(IN ULM_HANDLE hUlmHandle, IN UINT ulTimeOutTime )
{
    return _ULM_SetTimeOutTime(hUlmHandle, ulTimeOutTime);
}

BS_STATUS ULM_GetTimeOutTime(IN ULM_HANDLE hUlmHandle, OUT UINT *pulTimeOutTime )
{
    return _ULM_GetTimeOutTime(hUlmHandle, pulTimeOutTime);
}


BS_STATUS ULM_StopTimeOut(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId)
{
    return _ULM_StopTimeOut(hUlmHandle, ulUserId);
}


BS_STATUS ULM_StartTimeOut(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId)
{
    return _ULM_StartTimeOut(hUlmHandle, ulUserId);
}


BS_STATUS ULM_ResetTimeOut(IN ULM_HANDLE hUlmHandle, IN UINT ulUserId)
{
    return _ULM_ResetTimeOut(hUlmHandle, ulUserId);
}

BS_STATUS ULM_StopAllUserTimeOut(IN ULM_HANDLE hUlmHandle)
{
    return _ULM_StopAllUserTimeOut(hUlmHandle);
}

BS_STATUS ULM_StartAllUserTimeOut(IN ULM_HANDLE hUlmHandle)
{
    return _ULM_StartAllUserTimeOut(hUlmHandle);
}

VOID ULM_SetIfTimeOutIgnoreRef(IN ULM_HANDLE hUlmHandle, IN BOOL_T bIgnore)
{
    _ULM_INSTANCE_S *pstUlmInstance = hUlmHandle;

    BS_DBGASSERT(NULL != hUlmHandle);

    pstUlmInstance->bIsTimeOutIgnorRef = bIgnore;
}


VOID ULM_TimeOut(IN ULM_HANDLE hUlmHandle)
{
    _ULM_TimeOut(hUlmHandle);
}

