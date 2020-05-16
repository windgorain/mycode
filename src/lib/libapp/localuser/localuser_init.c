/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-31
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/object_utl.h"
#include "utl/mutex_utl.h"
#include "utl/exec_utl.h"
#include "utl/passwd_utl.h"
#include "utl/json_utl.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_localuser.h"

static NO_HANDLE g_hLocalUserNo;
static MUTEX_S g_stLocalUserMutex;
static COMP_LOCALUSER_S g_stLocalUserComp;

static CHAR *g_apcLocalUserProperty[] = {"Description"};
static UINT g_uiLocalUserPropertyCount = sizeof(g_apcLocalUserProperty)/sizeof(CHAR*);

static CHAR *g_apcLocalUserProperty1[] = {"Description", "Password"};
static UINT g_uiLocalUserPropertyCount1 = sizeof(g_apcLocalUserProperty1)/sizeof(CHAR*);

static BS_STATUS _localuser_EnterLocalUserView(IN CHAR *pcUserName)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLocalUserNo, pcUserName);
    if (pNode != NULL)
    {
        return BS_OK;
    }

    pNode = NO_NewObject(g_hLocalUserNo, pcUserName);
    if (NULL == pNode)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

static BS_STATUS _localuser_PasswordSimple(IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    VOID *pNode;
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];

    pNode = NO_GetObjectByName(g_hLocalUserNo, pcUserName);
    if (pNode == NULL)
    {
        return BS_NOT_FOUND;
    }

    PW_Md5Encrypt(pcPassword, szCipherText);

    return NO_SetKeyValue(g_hLocalUserNo, pNode, "Password", szCipherText);
}

static BS_STATUS _localuser_PasswordCipher(IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLocalUserNo, pcUserName);
    if (pNode == NULL)
    {
        return BS_NOT_FOUND;
    }

    return NO_SetKeyValue(g_hLocalUserNo, pNode, "Password", pcPassword);
}

static BS_STATUS _localuser_SetDescription(IN CHAR *pcUserName, IN CHAR *pcDesc)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLocalUserNo, pcUserName);
    if (pNode == NULL)
    {
        return BS_NOT_FOUND;
    }

    return NO_SetKeyValue(g_hLocalUserNo, pNode, "Description", pcDesc);
}


static VOID _localuser_SaveLocalUser(IN HANDLE hFile, IN UINT64 ulUserID)
{
    CHAR *pcTmp;

    pcTmp = NO_GetKeyValueByID(g_hLocalUserNo, ulUserID, "Password");
    if ((NULL != pcTmp) && (pcTmp[0] != '\0'))
    {
        CMD_EXP_OutputCmd(hFile, "password cipher %s", pcTmp);
    }

    pcTmp = NO_GetKeyValueByID(g_hLocalUserNo, ulUserID, "Description");
    if ((NULL != pcTmp) && (pcTmp[0] != '\0'))
    {
        CMD_EXP_OutputCmd(hFile, "description %s ", pcTmp);
    }
}

static VOID _localuser_Save(IN HANDLE hFile)
{
    UINT64 ulUserID = 0;
    CHAR *pcUserName;

    while ((ulUserID = NO_GetNextID(g_hLocalUserNo, ulUserID)) != 0)
    {
        pcUserName = NO_GetNameByID(g_hLocalUserNo, ulUserID);

        CMD_EXP_OutputMode(hFile, "local-user %s", pcUserName);
        _localuser_SaveLocalUser(hFile, ulUserID);
        CMD_EXP_OutputModeQuit(hFile);
    }
}

static LOCALUSER_RET_E _localuser_InnerLogin(IN CHAR *pcUserName, IN CHAR *pcPasswordInput)
{
    VOID *pNode;
    CHAR *pcPassword;
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];

#ifdef IN_UNIXLIKE
    /* 如果系统中不存在用户,则添加一个默认的admin用户 */
    if (NO_GetCount(g_hLocalUserNo) == 0)
    {
        pNode = NO_NewObject(g_hLocalUserNo, "admin");
        if (NULL == pNode)
        {
            return LOCALUSER_USER_NOT_EXIST;
        }

        _localuser_PasswordSimple("admin", "admin");
    }
#endif

    PW_Md5Encrypt(pcPasswordInput, szCipherText);

    pNode = NO_GetObjectByName(g_hLocalUserNo, pcUserName);
    if (pNode == NULL)
    {
        return LOCALUSER_USER_NOT_EXIST;
    }

    pcPassword = NO_GetKeyValue(pNode, "Password");
    if ((NULL == pcPassword) || (pcPassword[0] == '\0'))
    {
        return LOCALUSER_PASSWD_NOT_EXIST;
    }

    if (strcmp(szCipherText, pcPassword) == 0)
    {
        return LOCALUSER_OK;
    }

    return LOCALUSER_INVALID_PASSWD;
}

static LOCALUSER_RET_E _localuser_Login(IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    LOCALUSER_RET_E eRet;

    MUTEX_P(&g_stLocalUserMutex);
    eRet = _localuser_InnerLogin(pcUserName, pcPassword);
    MUTEX_V(&g_stLocalUserMutex);

    return eRet;
}

static VOID _localuser_InitComp()
{
    g_stLocalUserComp.pfLocalUserLogin = _localuser_Login;
    g_stLocalUserComp.comp.comp_name = COMP_LOCALUSER_NAME;

    COMP_Reg(&g_stLocalUserComp.comp);
}

static BS_STATUS _localuser_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stLocalUserMutex);
    eRet = JSON_NO_List(g_hLocalUserNo, hMime, pstParam->pstJson, g_apcLocalUserProperty, g_uiLocalUserPropertyCount);
    MUTEX_V(&g_stLocalUserMutex);

    return eRet;
}

static BS_STATUS _localuser_kf_IsExist(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    MUTEX_P(&g_stLocalUserMutex);
    JSON_NO_IsExist(g_hLocalUserNo, hMime, pstParam->pstJson);
    MUTEX_V(&g_stLocalUserMutex);

    return BS_OK;
}

static VOID _localusermf_PreAdd(IN MIME_HANDLE hMime)
{
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];
    CHAR *pcPassword;

    pcPassword = MIME_GetKeyValue(hMime, "Password");
    if (NULL == pcPassword)
    {
        return;
    }
    
    PW_Md5Encrypt(pcPassword, szCipherText);

    MIME_SetKeyValue(hMime, "Password", szCipherText);
}

static BS_STATUS _localuser_kf_Add(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    _localusermf_PreAdd(hMime);

    MUTEX_P(&g_stLocalUserMutex);
    JSON_NO_Add(g_hLocalUserNo, hMime, pstParam->pstJson, g_apcLocalUserProperty1, g_uiLocalUserPropertyCount1);
    MUTEX_V(&g_stLocalUserMutex);

    return BS_OK;
}

static BS_STATUS _localuser_kf_Modify(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    _localusermf_PreAdd(hMime);

    MUTEX_P(&g_stLocalUserMutex);
    JSON_NO_Modify(g_hLocalUserNo, hMime, pstParam->pstJson, g_apcLocalUserProperty1, g_uiLocalUserPropertyCount1);
    MUTEX_V(&g_stLocalUserMutex);

	return BS_OK;
}

static BS_STATUS _localuser_kf_Get(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    MUTEX_P(&g_stLocalUserMutex);
    JSON_NO_Get(g_hLocalUserNo, hMime, pstParam->pstJson, g_apcLocalUserProperty, g_uiLocalUserPropertyCount);
    MUTEX_V(&g_stLocalUserMutex);

	return BS_OK;
}

static BS_STATUS _localuser_kf_Del(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    MUTEX_P(&g_stLocalUserMutex);
    JSON_NO_Delete(g_hLocalUserNo, hMime, pstParam->pstJson);
    MUTEX_V(&g_stLocalUserMutex);

	return BS_OK;
}

static BS_STATUS _localuser_KfInit()
{
    COMP_KFAPP_RegFunc("localuser.IsExist", _localuser_kf_IsExist, NULL);
    COMP_KFAPP_RegFunc("localuser.Add", _localuser_kf_Add, NULL);
    COMP_KFAPP_RegFunc("localuser.Modify", _localuser_kf_Modify, NULL);
    COMP_KFAPP_RegFunc("localuser.Get", _localuser_kf_Get, NULL);
    COMP_KFAPP_RegFunc("localuser.Delete", _localuser_kf_Del, NULL);
    COMP_KFAPP_RegFunc("localuser.List", _localuser_kf_List, NULL);

    return BS_OK;
}

BS_STATUS LocalUser_Init()
{
    COMP_KFAPP_Init();
    
    g_hLocalUserNo = NO_CreateAggregate(0, 0, 0);

    if (NULL == g_hLocalUserNo)
    {
        return BS_ERR;
    }

    MUTEX_Init(&g_stLocalUserMutex);

    _localuser_InitComp();
    _localuser_KfInit();

    return BS_OK;
}

/* local-user xxx */
PLUG_API BS_STATUS LocalUser_EnterLocalUserView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stLocalUserMutex);
    eRet = _localuser_EnterLocalUserView(ppcArgv[1]);
    MUTEX_V(&g_stLocalUserMutex);
    
    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* password simple %STRING */
PLUG_API BS_STATUS LocalUser_CmdPasswordSimple(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcUserName;
    BS_STATUS eRet;

    pcUserName = CMD_EXP_GetCurrentModeValue(pEnv);

    MUTEX_P(&g_stLocalUserMutex);
    eRet = _localuser_PasswordSimple(pcUserName, ppcArgv[2]);
    MUTEX_V(&g_stLocalUserMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* password cipher %STRING */
PLUG_API BS_STATUS LocalUser_CmdPasswordCipher(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcUserName;
    BS_STATUS eRet;

    pcUserName = CMD_EXP_GetCurrentModeValue(pEnv);

    MUTEX_P(&g_stLocalUserMutex);
    eRet = _localuser_PasswordCipher(pcUserName, ppcArgv[2]);
    MUTEX_V(&g_stLocalUserMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* description %STRING */
PLUG_API BS_STATUS LocalUser_CmdDescription(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcUserName;
    BS_STATUS eRet;

    pcUserName = CMD_EXP_GetCurrentModeValue(pEnv);

    MUTEX_P(&g_stLocalUserMutex);
    eRet = _localuser_SetDescription(pcUserName, ppcArgv[1]);
    MUTEX_V(&g_stLocalUserMutex);

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

PLUG_API BS_STATUS LocalUser_Save(IN HANDLE hFile)
{
    MUTEX_P(&g_stLocalUserMutex);
    _localuser_Save(hFile);
    MUTEX_V(&g_stLocalUserMutex);

    return BS_OK;
}

