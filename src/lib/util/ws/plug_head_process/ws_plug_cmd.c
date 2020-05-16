/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-28
* Description: 在Query或者体中携带命令, 进行一些命令处理
    请求路径: /__ws_keep__/cmd.cgi
    命令格式: xxx=Action=x,Param1=value1,Param2=value2...&yyy=Action=y,Param1=value1,Param2=value2...
    value格式: 以HEX(开头,)结尾,则表示经过了十六进制编码
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/kv_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/ws_utl.h"

#include "../ws_def.h"
#include "../ws_conn.h"
#include "../ws_trans.h"
#include "../ws_event.h"
#include "../ws_context.h"

/*
    Action              Param                   说明
    SetCookie           CookieName=x+Value=y    设置Cookie: x=y
    EnterDomain         Domain=xxx              进入Domain
    Redirect2Url        Url=xxx                 重定向到xxx
*/

#define _WS_PLUG_CMD_URI "/__ws_keep__/cmd.cgi"

typedef BS_STATUS (*PF_WS_PLUG_CMD_FUNC)(IN WS_TRANS_S *pstTrans, IN KV_HANDLE hKv);

typedef struct
{
    CHAR *pcAction;
    PF_WS_PLUG_CMD_FUNC pfFunc;
}_WS_PLUG_CMD_ACTION_S;

static BS_STATUS ws_plugcmd_SetCookie(IN WS_TRANS_S *pstTrans, IN KV_HANDLE hKv);
static BS_STATUS ws_plugcmd_Redirect2Url(IN WS_TRANS_S *pstTrans, IN KV_HANDLE hKv);

static _WS_PLUG_CMD_ACTION_S g_astWsPlugCmdActions[] =
{
    {"SetCookie", ws_plugcmd_SetCookie},
    {"Redirect2Url", ws_plugcmd_Redirect2Url}
};

static BS_STATUS ws_plugcmd_Redirect2Url(IN WS_TRANS_S *pstTrans, IN KV_HANDLE hKv)
{
    CHAR *pcUrl;

    pcUrl = KV_GetKeyValue(hKv, "Url");
    if ((NULL == pcUrl) || (pcUrl[0] == '\0'))
    {
        return BS_ERR;
    }

    WS_Trans_Redirect(pstTrans, pcUrl);

    return BS_OK;
}

static BS_STATUS ws_plugcmd_SetCookie(IN WS_TRANS_S *pstTrans, IN KV_HANDLE hKv)
{
    CHAR *pcCookieName;
    CHAR *pcValue;
    CHAR szTmp[1024];

    pcCookieName = KV_GetKeyValue(hKv, "CookieName");
    if ((NULL == pcCookieName) || (pcCookieName[0] == '\0'))
    {
        return BS_ERR;
    }

    pcValue = KV_GetKeyValue(hKv, "Value");
    if (NULL == pcValue)
    {
        pcValue = "";
    }

    snprintf(szTmp, sizeof(szTmp), "%s=%s; path=/", pcCookieName, pcValue);

    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_SET_COOKIE, szTmp);

    return BS_OK;
}

#define WS_PLUGCMD_HEX_HEADER "HEX("
#define WS_PLUGCMD_HEX_HEADER_LEN STR_LEN(WS_PLUGCMD_HEX_HEADER)

static CHAR * ws_plugcmd_KvDecodeHex(IN LSTR_S *pstLstr)
{
    CHAR *pcData;
    UINT uiLen;
    CHAR *pcRet;
    UINT uiRetLen;
    
    if (pstLstr->uiLen < WS_PLUGCMD_HEX_HEADER_LEN)
    {
        return NULL;
    }

    pcData = pstLstr->pcData + WS_PLUGCMD_HEX_HEADER_LEN;
    uiLen = pstLstr->uiLen - WS_PLUGCMD_HEX_HEADER_LEN;

    if (uiLen > 0)
    {
        if (pcData[uiLen - 1] == ')')
        {
            uiLen --;
        }
    }

    uiRetLen = uiLen/2;

    pcRet = MEM_Malloc(uiRetLen + 1);
    if (NULL == pcRet)
    {
        return NULL;
    }

    if (BS_OK != DH_Data2Hex((UCHAR*)pcData, uiLen, pcRet))
    {
        MEM_Free(pcRet);
        return NULL;
    }

    pcRet[uiRetLen] = '\0';

    return pcRet;
}

static CHAR * ws_pluccmd_KvDecode(IN LSTR_S *pstLstr)
{
    CHAR *pcTmp;
    
    if ((pstLstr->uiLen > WS_PLUGCMD_HEX_HEADER_LEN)
        && (strnicmp(pstLstr->pcData, WS_PLUGCMD_HEX_HEADER, WS_PLUGCMD_HEX_HEADER_LEN) == 0))
    {
        return ws_plugcmd_KvDecodeHex(pstLstr);
    }
    else
    {
        pcTmp = MEM_Malloc(pstLstr->uiLen + 1);

        if (NULL != pcTmp)
        {
            memcpy(pcTmp, pstLstr->pcData, pstLstr->uiLen);
            pcTmp[pstLstr->uiLen] = '\0';
        }

        return pcTmp;
    }
}

static BS_STATUS ws_plugcmd_DoAction(IN WS_TRANS_S *pstTrans, IN CHAR *pcActionString)
{
    KV_HANDLE hKv;
    LSTR_S stLstr;
    CHAR *pcAction;
    UINT i;
    BS_STATUS eRet = BS_NOT_FOUND;

    stLstr.pcData = pcActionString;
    stLstr.uiLen = strlen(pcActionString);

    hKv = KV_Create(0);
    if (hKv == NULL)
    {
        return BS_ERR;
    }

    KV_SetDecode(hKv, ws_pluccmd_KvDecode);

    KV_Parse(hKv, &stLstr, ',', '=');
    pcAction = KV_GetKeyValue(hKv, "Action");

    if ((NULL != pcAction) && (pcAction[0] != '\0'))
    {
        for (i=0; i<sizeof(g_astWsPlugCmdActions)/sizeof(_WS_PLUG_CMD_ACTION_S); i++)
        {
            if (strcmp(pcAction, g_astWsPlugCmdActions[i].pcAction) == 0)
            {
                eRet = g_astWsPlugCmdActions[i].pfFunc(pstTrans, hKv);
                break;
            }
        }
    }

    KV_Destory(hKv);

    return eRet;
}

static BS_STATUS ws_plugcmd_RunCmd(IN WS_TRANS_S *pstTrans)
{
    MIME_HANDLE hMime;
    MIME_DATA_NODE_S *pstParam = NULL;
    

    hMime = pstTrans->hBodyMime;
    if (hMime == NULL)
    {
        hMime = pstTrans->hQuery;
        if (NULL == hMime)
        {
            return BS_OK;
        }
    }

    while ((pstParam = MIME_GetNextParam(hMime, pstParam)) != NULL)
    {
        ws_plugcmd_DoAction(pstTrans, pstParam->pcValue);
    }

    return BS_OK;
}

WS_EV_RET_E _WS_PlugCmd_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    CHAR *pcUrl;
    
    pcUrl = HTTP_GetUriAbsPath(pstTrans->hHttpHeadRequest);
    if (NULL == pcUrl)
    {
        return WS_EV_RET_ERR;
    }
    
    if (strcmp(pcUrl, _WS_PLUG_CMD_URI) != 0)
    {
        return WS_EV_RET_CONTINUE;
    }

    HTTP_SetStatusCode(pstTrans->hHttpHeadReply, HTTP_STATUS_OK);
    _WS_Trans_SetFlag(pstTrans, WS_TRANS_FLAG_DROP_REQ_BODY | WS_TRANS_FLAG_REPLY_AFTER_BODY);

    ws_plugcmd_RunCmd(pstTrans);

    WS_Trans_SetHeadFieldFinish(pstTrans);
    WS_Trans_ReplyBodyFinish(pstTrans);

    return WS_EV_RET_STOP;
}

