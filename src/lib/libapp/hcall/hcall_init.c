/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-8-29
* Description: HTTP Call: 使用http协议, 发送Action
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/bit_opt.h"
#include "utl/mutex_utl.h"
#include "utl/str_list.h"
#include "comp/comp_wsapp.h"
#include "comp/comp_kfapp.h"

#define HCALL_DBG_PACKET 0x1


static CHAR g_szHcallWsService[WSAPP_SERVICE_NAME_LEN + 1] = "";
static WS_DELIVER_TBL_HANDLE g_hHCallDeliverTbl = NULL;
static UINT g_uiHCallDbgFlag = 0;
static STRLIST_HANDLE g_hHCallModuleList = NULL;
static CMD_EXP_NO_DBG_NODE_S g_stHCallNoDbgNode;
static MUTEX_S g_stHCallMutex;

static LSTR_S * _hcall_GetModuleName(IN CHAR *pcAction, OUT LSTR_S *pstLstr)
{
    CHAR *pcSplit;
    
    pstLstr->pcData = pcAction;
    pcSplit = strchr(pcAction, '.');
    if (NULL == pcSplit)
    {
        pstLstr->uiLen = strlen(pcAction);
    }
    else
    {
        pstLstr->uiLen = pcSplit - pcAction;
    }

    if (pstLstr->uiLen == 0)
    {
        return NULL;
    }

    return pstLstr;
}

static BOOL_T _hcall_IsPermit(IN CHAR *pcAction)
{
    BOOL_T bPermit = FALSE;
    LSTR_S stModuleName;

    if ((NULL == pcAction) || (pcAction[0] == '\0'))
    {
        return FALSE;
    }

    if (NULL == _hcall_GetModuleName(pcAction, &stModuleName))
    {
        return FALSE;
    }

    MUTEX_P(&g_stHCallMutex);
    if (NULL != StrList_FindByLstr(g_hHCallModuleList, &stModuleName))
    {
        bPermit = TRUE;
    }
    MUTEX_V(&g_stHCallMutex);

    return bPermit;
}

static BS_STATUS _hcall_kf_Run(IN WS_TRANS_HANDLE hWsTrans, IN KFAPP_PARAM_S *pstParam)
{
    MIME_HANDLE hMime;

    hMime = WS_Trans_GetBodyMime(hWsTrans);
    if (NULL == hMime)
    {
        hMime = WS_Trans_GetQueryMime(hWsTrans);
    }

    if (NULL == hMime)
    {
        return BS_OK;
    }

    if (TRUE != _hcall_IsPermit(MIME_GetKeyValue(hMime, "_do")))
    {
        return BS_NO_PERMIT;
    }

    return KFAPP_RunMime(hMime, pstParam);
}

static BS_STATUS _hcall_RecvBodyOK(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    KFAPP_PARAM_S stKfappParam;
    BS_STATUS eRet;
    HTTP_HEAD_PARSER hParser;
    CHAR *pcQuery;

    if (g_uiHCallDbgFlag & HCALL_DBG_PACKET)
    {
        hParser = WS_Trans_GetHttpRequestParser(hWsTrans);
        pcQuery = HTTP_GetUriQuery(hParser);
        if (NULL == pcQuery)
        {
            pcQuery = "";
        }
        BS_DBG_OUTPUT(g_uiHCallDbgFlag, HCALL_DBG_PACKET, ("HCall: request query:%s\r\n", pcQuery));
    }

    if (BS_OK != KFAPP_ParamInit(&stKfappParam))
    {
        return BS_NO_MEMORY;
    }

    eRet = _hcall_kf_Run(hWsTrans, &stKfappParam);
    if (eRet != BS_OK)
    {
        KFAPP_ParamFini(&stKfappParam);
        return BS_ERR;
    }

    if (NULL == KFAPP_BuildParamString(&stKfappParam))
    {
        KFAPP_ParamFini(&stKfappParam);
        return BS_NO_MEMORY;
    }

    BS_DBG_OUTPUT(g_uiHCallDbgFlag, HCALL_DBG_PACKET, ("HCall: reply:%s\r\n", stKfappParam.pcString));

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, stKfappParam.uiStringLen);
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(hWsTrans);

    WS_Trans_AddReplyBodyByBuf(hWsTrans, (void*)stKfappParam.pcString, stKfappParam.uiStringLen);
    WS_Trans_ReplyBodyFinish(hWsTrans);

    KFAPP_ParamFini(&stKfappParam);

    return BS_OK;
}

static WS_DELIVER_RET_E _hcall_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = _hcall_RecvBodyOK(hTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    if (eRet != BS_OK)
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}

static BS_STATUS _hcall_DeliverInit()
{
    g_hHCallDeliverTbl = WS_Deliver_Create();
    if (NULL == g_hHCallDeliverTbl)
    {
        return BS_NO_MEMORY;
    }

    WS_Deliver_Reg(g_hHCallDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/request.cgi", _hcall_RequestIn, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    return BS_OK;
}

static VOID _hcall_NoDbgAll()
{
    g_uiHCallDbgFlag = 0;
}

BS_STATUS HCall_Init()
{
    g_stHCallNoDbgNode.pfNoDbgFunc = _hcall_NoDbgAll;
    CMD_EXP_RegNoDbgFunc(&g_stHCallNoDbgNode);

    g_hHCallModuleList = StrList_Create(STRLIST_FLAG_CASE_SENSITIVE | STRLIST_FLAG_CHECK_REPEAT);
    if (NULL == g_hHCallModuleList)
    {
        return BS_ERR;
    }

    MUTEX_Init(&g_stHCallMutex);

    _hcall_DeliverInit();

    return BS_OK;
}


PLUG_API BS_STATUS HCall_BindService(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    if (BS_OK != WSAPP_BindService(argv[2]))
    {
        EXEC_OutString("Bind failed");
        return BS_ERR;
    }

    WSAPP_SetDeliverTbl(argv[2], g_hHCallDeliverTbl);

    if (g_szHcallWsService[0] != '\0')
    {
        WSAPP_UnBindService(g_szHcallWsService);
    }

    TXT_Strlcpy(g_szHcallWsService, argv[2], sizeof(g_szHcallWsService));

    return BS_OK;
}


PLUG_API BS_STATUS HCall_DebugPacket(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    if (argv[0][0] == 'n')
    {
        BIT_CLR(g_uiHCallDbgFlag, HCALL_DBG_PACKET);
    }
    else
    {
        BIT_SET(g_uiHCallDbgFlag, HCALL_DBG_PACKET);
    }
    
    return BS_OK;
}


PLUG_API BS_STATUS HCall_UnBindService(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    if (g_szHcallWsService[0] != '\0')
    {
        WSAPP_UnBindService(g_szHcallWsService);
    }

    g_szHcallWsService[0] = '\0';

    return BS_OK;
}


PLUG_API BS_STATUS HCall_PermitModule(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    BS_STATUS eRet = BS_OK;

    MUTEX_P(&g_stHCallMutex);
    if (argv[0][0] == 'n')
    {
        StrList_Del(g_hHCallModuleList, argv[3]);
    }
    else
    {
        eRet=  StrList_Add(g_hHCallModuleList, argv[2]);
    }
    MUTEX_V(&g_stHCallMutex);

    return eRet;
}

PLUG_API BS_STATUS HCall_Save(IN HANDLE hFile)
{
    STRLIST_NODE_S *pstNode = NULL;

    if (g_szHcallWsService[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFile, "bind ws-service %s", g_szHcallWsService);
    }

    MUTEX_P(&g_stHCallMutex);
    while ((pstNode = StrList_GetNext(g_hHCallModuleList, pstNode)) != NULL)
    {
		CMD_EXP_OutputCmd(hFile, "permit module %s", pstNode->stStr.pcData);
    }
    MUTEX_V(&g_stHCallMutex);

    return BS_OK;
}

