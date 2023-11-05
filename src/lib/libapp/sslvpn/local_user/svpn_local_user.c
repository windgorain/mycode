/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-6
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/passwd_utl.h"
#include "utl/txt_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"
#include "../h/svpn_ulm.h"

static inline BS_STATUS svpn_localuser_AddUser
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcUserName,
    IN CHAR *pcPassword
)
{
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];
    BS_STATUS eRet;
    
    PW_Md5Encrypt(pcPassword, szCipherText);

    eRet = SVPN_CtxData_AddObject(hSvpnContext, enDataIndex, pcUserName);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    return SVPN_CtxData_SetProp(hSvpnContext, enDataIndex, pcUserName, "Password", szCipherText);
}

static inline BS_STATUS svpn_localuser_SetUser
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcUserName,
    IN CHAR *pcPassword
)
{
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];
    BS_STATUS eRet;
    
    if (! SVPN_CtxData_IsObjectExist(hSvpnContext, enDataIndex, pcUserName))
    {
        eRet = SVPN_CtxData_AddObject(hSvpnContext, enDataIndex, pcUserName);
        if (BS_OK != eRet)
        {
            return eRet;
        }
    }

    PW_Md5Encrypt(pcPassword, szCipherText);

    return SVPN_CtxData_SetProp(hSvpnContext, enDataIndex, pcUserName, "Password", szCipherText);
}


static inline BOOL_T svpn_localuser_Check
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR *pcUserName,
    IN CHAR *pcPassword
)
{
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];
    CHAR szPasswordSaved[PW_MD5_ENCRYPT_LEN + 1];
    BOOL_T bCheck = FALSE;

    if (BS_OK != SVPN_CtxData_GetProp(hSvpnContext, enDataIndex, pcUserName,
        "Password", szPasswordSaved, sizeof(szPasswordSaved)))
    {
        return FALSE;
    }

    PW_Md5Encrypt(pcPassword, szCipherText);

    if (strcmp(szCipherText, szPasswordSaved) == 0)
    {
        bCheck = TRUE;
    }

    return bCheck;
}

BS_STATUS SVPN_LocalUser_AddUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    return svpn_localuser_AddUser(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, pcUserName, pcPassword);
}

VOID SVPN_LocalUser_DelUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName)
{
    SVPN_CtxData_DelObject(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, pcUserName);
}

BOOL_T SVPN_LocalUser_Check(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    return svpn_localuser_Check(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, pcUserName, pcPassword);
}

BOOL_T SVPN_LocalUser_IsExist(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName)
{
    return SVPN_CtxData_IsObjectExist(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, pcUserName);
}

BS_STATUS SVPN_LocalUser_GetNext
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcCurrentUserName,
    OUT CHAR szNextUserName[SVPN_MAX_USER_NAME_LEN + 1]
)
{
    return SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_LOCAL_USER,
        pcCurrentUserName, szNextUserName, SVPN_MAX_USER_NAME_LEN + 1);
}

HSTRING SVPN_LocalUser_GetRoleAsHString(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName)
{
    return SVPN_CtxData_GetPropAsHString(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, pcUserName, "Role");
}

BS_STATUS SVPN_LocalUser_SetRole(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcRole)
{
    return SVPN_CtxData_SetProp(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, pcUserName, "Role", pcRole);
}

static BS_STATUS svpn_localuser_SetPassword(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];
    
    PW_Md5Encrypt(pcPassword, szCipherText);
    
    return SVPN_CtxData_SetProp(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, pcUserName, "Password", szCipherText);
}

BS_STATUS SVPN_LocalAdmin_AddUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    return svpn_localuser_AddUser(hSvpnContext, SVPN_CTXDATA_ADMIN, pcUserName, pcPassword);
}

VOID SVPN_LocalAdmin_DelUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName)
{
    SVPN_CtxData_DelObject(hSvpnContext, SVPN_CTXDATA_ADMIN, pcUserName);
}

BS_STATUS SVPN_LocalAdmin_SetUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    return svpn_localuser_SetUser(hSvpnContext, SVPN_CTXDATA_ADMIN, pcUserName, pcPassword);
}

BOOL_T SVPN_LocalAdmin_Check(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    return svpn_localuser_Check(hSvpnContext, SVPN_CTXDATA_ADMIN, pcUserName, pcPassword);
}

BOOL_T SVPN_LocalAdmin_IsExist(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName)
{
    return SVPN_CtxData_IsObjectExist(hSvpnContext, SVPN_CTXDATA_ADMIN, pcUserName);
}

BS_STATUS SVPN_LocalAdmin_GetNext
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcCurrentUserName,
    OUT CHAR szNextUserName[SVPN_MAX_USER_NAME_LEN + 1]
)
{
    return SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_ADMIN, pcCurrentUserName, szNextUserName, SVPN_MAX_USER_NAME_LEN + 1);
}



static CHAR *g_apcSvpnLocalUserProperty[] = {"Password", "Role"};
static UINT g_uiSvpnLocalUserPropertyCount = sizeof(g_apcSvpnLocalUserProperty)/sizeof(CHAR*);

static CHAR *g_apcSvpnLocalUserProperty1[] = {"Role"};
static UINT g_uiSvpnLocalUserPropertyCount1 = sizeof(g_apcSvpnLocalUserProperty1)/sizeof(CHAR*);

static VOID svpn_localusermf_ChangePwd(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    CHAR *pcOldPwd;
    CHAR *pcNewPwd;
    CHAR szUserName[SVPN_MAX_USER_NAME_LEN + 1];

    pcOldPwd = MIME_GetKeyValue(hMime, "OldPassword");
    pcNewPwd = MIME_GetKeyValue(hMime, "Password");
    if ((NULL == pcOldPwd) || (NULL == pcNewPwd) || (pcOldPwd[0] == '\0') || (pcNewPwd[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Password is empty");
        return;
    }

    if (BS_OK != SVPN_ULM_GetUserName(pstDweb->hSvpnContext, pstDweb->uiOnlineUserID, szUserName))
    {
        svpn_mf_SetFailed(pstDweb, "User is not exist");
        return;
    }

    if (TRUE != SVPN_LocalUser_Check(pstDweb->hSvpnContext, szUserName, pcOldPwd))
    {
        svpn_mf_SetFailed(pstDweb, "Password error");
        return;
    }

    if (BS_OK != svpn_localuser_SetPassword(pstDweb->hSvpnContext, szUserName, pcNewPwd))
    {
        svpn_mf_SetFailed(pstDweb, "Set user password failed");
        return;
    }

    svpn_mf_SetSuccess(pstDweb);

    return;
}

static VOID svpn_localusermf_IsExist(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonIsExist(hMime, pstDweb, SVPN_CTXDATA_LOCAL_USER);
}

static VOID svpn_localusermf_PreProcess(IN MIME_HANDLE hMime)
{
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];
    CHAR *pcPassword;

    pcPassword = MIME_GetKeyValue(hMime, "Password");
    if (NULL == pcPassword)
    {
        pcPassword = "";
    }
    
    PW_Md5Encrypt(pcPassword, szCipherText);

    MIME_SetKeyValue(hMime, "Password", szCipherText);
}

static VOID svpn_localusermf_Add(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    svpn_localusermf_PreProcess(hMime);
    
    SVPN_MF_CommonAdd(hMime, pstDweb, SVPN_CTXDATA_LOCAL_USER, g_apcSvpnLocalUserProperty, g_uiSvpnLocalUserPropertyCount);
}

static VOID svpn_localusermf_Del(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonDelete(hMime, pstDweb, SVPN_CTXDATA_LOCAL_USER, NULL);
}

static VOID svpn_localuser_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    svpn_localusermf_PreProcess(hMime);
    
    SVPN_MF_CommonModify(hMime, pstDweb, SVPN_CTXDATA_LOCAL_USER, g_apcSvpnLocalUserProperty, g_uiSvpnLocalUserPropertyCount);
}

static VOID svpn_localusermf_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonGet(hMime, pstDweb, SVPN_CTXDATA_LOCAL_USER, g_apcSvpnLocalUserProperty1, g_uiSvpnLocalUserPropertyCount1);
}

static VOID svpn_localusermf_List(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonList(hMime, pstDweb, SVPN_CTXDATA_LOCAL_USER, g_apcSvpnLocalUserProperty1, g_uiSvpnLocalUserPropertyCount1);
}

static SVPN_MF_MAP_S g_astSvpnLocalUserMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "LocalUser.IsExist",  svpn_localusermf_IsExist},
    {SVPN_USER_TYPE_ADMIN, "LocalUser.Add",      svpn_localusermf_Add},
    {SVPN_USER_TYPE_ADMIN, "LocalUser.Delete",   svpn_localusermf_Del},
    {SVPN_USER_TYPE_ADMIN, "LocalUser.Modify",   svpn_localuser_Modify},
    {SVPN_USER_TYPE_ADMIN, "LocalUser.Get",      svpn_localusermf_Get},
    {SVPN_USER_TYPE_ADMIN, "LocalUser.List",     svpn_localusermf_List},
    {SVPN_USER_TYPE_USER,  "User.Password.Change", svpn_localusermf_ChangePwd}, 
};

BS_STATUS SVPN_LocalUser_Init()
{
    return SVPN_MF_Reg(g_astSvpnLocalUserMfMap, sizeof(g_astSvpnLocalUserMfMap)/sizeof(SVPN_MF_MAP_S));
}





PLUG_API BS_STATUS SVPN_LocalUserCmd_EnterView(UINT argc,
        char **argv, VOID *pEnv)
{
    return SVPN_CD_EnterView(pEnv, SVPN_CTXDATA_LOCAL_USER, argv[1]);
}


PLUG_API BS_STATUS SVPN_LocalUserCmd_SetPassword(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];
    
    PW_Md5Encrypt(argv[2], szCipherText);

    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_LOCAL_USER, "Password", szCipherText);
}


PLUG_API BS_STATUS SVPN_LocalUserCmd_SetPasswordCipher(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_LOCAL_USER, "Password", argv[2]);
}


PLUG_API BS_STATUS SVPN_LocalUserCmd_SetRole(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_LOCAL_USER, "Role", argv[1], SVPN_PROPERTY_SPLIT);
}

BS_STATUS SVPN_LocalUser_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile)
{
    CHAR szUserName[SVPN_MAX_USER_NAME_LEN + 1] = "";

    while (BS_OK == SVPN_LocalUser_GetNext(hSvpnContext, szUserName, szUserName))
    {
        if (0 != CMD_EXP_OutputMode(hFile, "local-user %s", szUserName)) {
            continue;
        }

        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, szUserName, "Password", "password cipher", hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, szUserName, "Role", "role", SVPN_PROPERTY_SPLIT, hFile);

        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}

