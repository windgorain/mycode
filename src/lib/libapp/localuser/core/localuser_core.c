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
#include "utl/txt_utl.h"
#include "utl/passwd_utl.h"
#include "utl/json_utl.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_localuser.h"

static NO_HANDLE g_hLocalUserNo;
static MUTEX_S g_stLocalUserMutex;

static CHAR *g_apcLocalUserProperty[] = {"Description", NULL};

static CHAR *g_apcLocalUserProperty1[] = {"Description", "Password", NULL};

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

static UINT _localuser_Str2Type(char *type_str)
{
    char *tmp = type_str;
    UINT type = 0;

    while (*tmp != '\0') {
        switch (*tmp) {
            case 'c':
                type |= CMD_EXP_RUNNER_TYPE_CMD;
                break;
            case 'p':
                type |= CMD_EXP_RUNNER_TYPE_PIPECMD;
                break;
            case 't':
                type |= CMD_EXP_RUNNER_TYPE_TELNET;
                break;
            case 'w':
                type |= CMD_EXP_RUNNER_TYPE_WEB;
                break;
        }
        tmp ++;
    }

    return type;
}

static void _localuser_Type2Str(UINT type, OUT char *type_str)
{
    if (type & CMD_EXP_RUNNER_TYPE_CMD) {
        *type_str = 'c';
        type_str++;
    }

    if (type & CMD_EXP_RUNNER_TYPE_TELNET) {
        *type_str = 't';
        type_str++;
    }

    if (type & CMD_EXP_RUNNER_TYPE_PIPECMD) {
        *type_str = 'p';
        type_str++;
    }

    if (type & CMD_EXP_RUNNER_TYPE_WEB) {
        *type_str = 'w';
        type_str++;
    }

    *type_str = '\0';
}

static char * _localuser_Type2FullStr(UINT type, OUT char *type_str)
{
    char *tmp = type_str;

    if (type & CMD_EXP_RUNNER_TYPE_CMD) {
        tmp += sprintf(tmp, "cmd ");
    }

    if (type & CMD_EXP_RUNNER_TYPE_TELNET) {
        tmp += sprintf(tmp, "telnet ");
    }

    if (type & CMD_EXP_RUNNER_TYPE_PIPECMD) {
        tmp += sprintf(tmp, "pipecmd ");
    }

    if (type & CMD_EXP_RUNNER_TYPE_WEB) {
        tmp += sprintf(tmp, "web ");
    }

    return type_str;
}

static int _localuser_ParseType(int argc, char **argv, OUT char *type_str)
{
    int i;
    UINT type = 0;

    for (i=0; i<argc; i++) {
        if (strcmp(argv[i], "telnet") == 0) {
            type |= CMD_EXP_RUNNER_TYPE_TELNET;
        } else if (strcmp(argv[i], "cmd") == 0) {
            type |= CMD_EXP_RUNNER_TYPE_CMD;
        } else if (strcmp(argv[i], "pipecmd") == 0) {
            type |= CMD_EXP_RUNNER_TYPE_PIPECMD;
        } else if (strcmp(argv[i], "web") == 0) {
            type |= CMD_EXP_RUNNER_TYPE_WEB;
        }
    }

    _localuser_Type2Str(type, type_str);

    return 0;
}

static UINT _localuser_GetUserType(char *user)
{
    char * type_str = NO_GetKeyValueByName(g_hLocalUserNo, user, "type");
    if (type_str == NULL) {
        return 0;
    }

    return _localuser_Str2Type(type_str);
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

static BS_STATUS _localuser_SetType(char *pcUserName, char *type)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLocalUserNo, pcUserName);
    if (pNode == NULL) {
        return BS_NOT_FOUND;
    }

    return NO_SetKeyValue(g_hLocalUserNo, pNode, "type", type);
}

static VOID _localuser_SaveLocalUser(IN HANDLE hFile, IN UINT64 ulUserID)
{
    CHAR *pcTmp;
    char info[128];

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

    pcTmp = NO_GetKeyValueByID(g_hLocalUserNo, ulUserID, "type");
    if ((NULL != pcTmp) && (pcTmp[0] != '\0')) {
        UINT type = _localuser_Str2Type(pcTmp);
        if (type != 0) {
            CMD_EXP_OutputCmd(hFile, "type %s ", _localuser_Type2FullStr(type, info));
        }
    }
}

static VOID _localuser_Save(IN HANDLE hFile)
{
    UINT64 ulUserID = 0;
    CHAR *pcUserName;

    while ((ulUserID = NO_GetNextID(g_hLocalUserNo, ulUserID)) != 0) {
        pcUserName = NO_GetNameByID(g_hLocalUserNo, ulUserID);

        if (0 == CMD_EXP_OutputMode(hFile, "local-user %s", pcUserName)) {
            _localuser_SaveLocalUser(hFile, ulUserID);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }
}

static LOCALUSER_RET_E _localuser_InnerLogin(UINT type, char *pcUserName, char *pcPasswordInput)
{
    VOID *pNode;
    CHAR *pcPassword;
    CHAR szCipherText[PW_MD5_ENCRYPT_LEN + 1];

#ifdef IN_UNIXLIKE
    
    if (NO_GetCount(g_hLocalUserNo) == 0) {
        pNode = NO_NewObject(g_hLocalUserNo, "admin");
        if (NULL == pNode) {
            return LOCALUSER_USER_NOT_EXIST;
        }

        _localuser_PasswordSimple("admin", "admin");
    }
#endif

    UINT user_type = _localuser_GetUserType(pcUserName);
    if ((user_type & type) == 0) {
        return LOCALUSER_USER_NOT_EXIST;
    }

    PW_Md5Encrypt(pcPasswordInput, szCipherText);

    pNode = NO_GetObjectByName(g_hLocalUserNo, pcUserName);
    if (pNode == NULL) {
        return LOCALUSER_USER_NOT_EXIST;
    }

    pcPassword = NO_GetKeyValue(pNode, "Password");
    if ((NULL == pcPassword) || (pcPassword[0] == '\0')) {
        return LOCALUSER_PASSWD_NOT_EXIST;
    }

    if (strcmp(szCipherText, pcPassword) == 0) {
        return LOCALUSER_OK;
    }

    return LOCALUSER_INVALID_PASSWD;
}

PLUG_API LOCALUSER_RET_E LocalUser_Login(UINT type, char *pcUserName, char *pcPassword)
{
    LOCALUSER_RET_E eRet;

    MUTEX_P(&g_stLocalUserMutex);
    eRet = _localuser_InnerLogin(type, pcUserName, pcPassword);
    MUTEX_V(&g_stLocalUserMutex);

    return eRet;
}

static BS_STATUS _localuser_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stLocalUserMutex);
    eRet = JSON_NO_List(g_hLocalUserNo, pstParam->pstJson, g_apcLocalUserProperty);
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
    JSON_NO_Add(g_hLocalUserNo, hMime, pstParam->pstJson, g_apcLocalUserProperty1);
    MUTEX_V(&g_stLocalUserMutex);

    return BS_OK;
}

static BS_STATUS _localuser_kf_Modify(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    _localusermf_PreAdd(hMime);

    MUTEX_P(&g_stLocalUserMutex);
    JSON_NO_Modify(g_hLocalUserNo, hMime, pstParam->pstJson, g_apcLocalUserProperty1);
    MUTEX_V(&g_stLocalUserMutex);

	return BS_OK;
}

static BS_STATUS _localuser_kf_Get(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    MUTEX_P(&g_stLocalUserMutex);
    JSON_NO_Get(g_hLocalUserNo, hMime, pstParam->pstJson, g_apcLocalUserProperty);
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
    KFAPP_RegFunc("localuser.IsExist", _localuser_kf_IsExist, NULL);
    KFAPP_RegFunc("localuser.Add", _localuser_kf_Add, NULL);
    KFAPP_RegFunc("localuser.Modify", _localuser_kf_Modify, NULL);
    KFAPP_RegFunc("localuser.Get", _localuser_kf_Get, NULL);
    KFAPP_RegFunc("localuser.Delete", _localuser_kf_Del, NULL);
    KFAPP_RegFunc("localuser.List", _localuser_kf_List, NULL);

    return BS_OK;
}

static BOOL_T _localuser_IsTypeUserExistLocked(UINT type)
{
    UINT64 ulUserID = 0;
    char *type_str;
    UINT type_flag;

    while ((ulUserID = NO_GetNextID(g_hLocalUserNo, ulUserID)) != 0) {
        type_str = NO_GetKeyValueByID(g_hLocalUserNo, ulUserID, "type");
        if (type_str == NULL) {
            continue;
        }

        type_flag = _localuser_Str2Type(type_str);
        if (type_flag & type) {
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL_T _localuser_IsTypeUserExist(UINT type)
{
    BOOL_T ret;

    MUTEX_P(&g_stLocalUserMutex);
    ret = _localuser_IsTypeUserExistLocked(type);
    MUTEX_V(&g_stLocalUserMutex);

    return ret;
}

BS_STATUS LocalUserCore_Init()
{
    OBJECT_PARAM_S no_param = {0};

    g_hLocalUserNo = NO_CreateAggregate(&no_param);

    if (NULL == g_hLocalUserNo)
    {
        return BS_ERR;
    }

    MUTEX_Init(&g_stLocalUserMutex);

    _localuser_KfInit();

    return BS_OK;
}


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


PLUG_API int LocalUser_CmdType(int argc, char **argv, void *env)
{
    CHAR *pcUserName;
    BS_STATUS eRet;
    char type[32];

    pcUserName = CMD_EXP_GetCurrentModeValue(env);

    _localuser_ParseType(argc - 1, argv + 1, type);

    MUTEX_P(&g_stLocalUserMutex);
    eRet = _localuser_SetType(pcUserName, type);
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

typedef struct {
    char user[128];
}LOCAL_USER_ENTER_NODE_S;

static int _localuser_RunnerCmd(char *line, void *ud, void *env)
{
    LOCAL_USER_ENTER_NODE_S *node = ud;
    void *runner = CmdExp_GetEnvRunner(env);
    UINT runner_type = CmdExp_GetRunnerType(runner);

    line = TXT_Strim(line);

    if (node->user[0] == '\0') {
        strlcpy(node->user, line, sizeof(node->user));
        if (node->user[0] == '\0') {
            EXEC_OutString("\r\nUser: ");
        } else {
            EXEC_OutString("\r\nPassword: ");
            CmdExp_AltEnable(runner, FALSE);
        }
        EXEC_Flush();
        return BS_STOLEN;
    }

    if (LOCALUSER_OK != LocalUser_Login(runner_type, node->user, line)) {
        CmdExp_QuitMode(runner);
    }

    CmdExp_AltEnable(runner, TRUE);
    CmdExp_SetRunnerHookMode(runner, FALSE);
    CmdExp_SetRunnerHook(runner, NULL, NULL);

    EXEC_OutString("\r\n");
    CmdExp_RunnerOutputPrefix(runner);

    return BS_STOLEN;
}

static int _localuser_CmdHook(int event, void *data, void *ud, void *env)
{
    int ret = 0;

    switch (event) {
        case CMD_EXP_HOOK_EVENT_LINE:
            ret = _localuser_RunnerCmd(data, ud, env);
            break;
        case CMD_EXP_HOOK_EVENT_DESTROY:
            MEM_Free(ud);
            break;
    }

    return ret;
}

PLUG_API int LocalUser_Enter(void *env)
{
    LOCAL_USER_ENTER_NODE_S *pstNode;

    if (PLUGCT_GetLoadStage() < PLUG_STAGE_RUNNING) {
        return 0;
    }

    void *runner = CmdExp_GetEnvRunner(env);
    UINT runner_type = CmdExp_GetRunnerType(runner);

    if (runner_type == CMD_EXP_RUNNER_TYPE_NONE) {
        return 0;
    }

    if (! _localuser_IsTypeUserExist(runner_type)) {
        return 0;
    }

    pstNode = MEM_ZMalloc(sizeof(LOCAL_USER_ENTER_NODE_S));

    CmdExp_SetRunnerHook(runner, _localuser_CmdHook, pstNode);

    CmdExp_SetRunnerHookMode(runner, TRUE);

    EXEC_OutString("User: ");
    EXEC_Flush();

    return 0;
}


