/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-9-28
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/ws_utl.h"

#include "ws_def.h"
#include "ws_conn.h"
#include "ws_trans.h"
#include "ws_event.h"


typedef struct
{
    CHAR *pcLisenserName;
    UINT uiCareEvents;
    PF_WS_EventProcess pfFunc;
}_WS_EVENT_S;

typedef struct
{
    CHAR *pcStage;  /* 阶段 */
    _WS_EVENT_S *pstEventPlugs;
    UINT uiPlugNum;
}_WS_EVENT_TBL_S;

extern WS_EV_RET_E _WS_PlugCmd_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugIndex_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugContext_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugSetContext_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugStatic_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugDeliver_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugDWeb_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugContentType_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);
extern WS_EV_RET_E _WS_PlugConnType_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent);

static _WS_EVENT_S g_astWsHeadProcess[] =
{
    {"WsCmd", WS_TRANS_EVENT_RECV_HEAD_OK, _WS_PlugCmd_EventProcess},
    {"SetContext", WS_TRANS_EVENT_RECV_HEAD_OK, _WS_PlugSetContext_EventProcess},
    {"Context", WS_TRANS_EVENT_RECV_HEAD_OK, _WS_PlugContext_EventProcess},
    {"Index", WS_TRANS_EVENT_RECV_HEAD_OK, _WS_PlugIndex_EventProcess},
};

static _WS_EVENT_S g_astWsHandler[] =
{
    {"Deliver",
        WS_TRANS_EVENT_RECV_HEAD_OK
        | WS_TRANS_EVENT_RECV_BODY
        | WS_TRANS_EVENT_RECV_BODY_OK
        | WS_TRANS_EVENT_BUILD_BODY
        | WS_TRANS_EVENT_DESTORY,
        _WS_PlugDeliver_EventProcess},

    {"dweb",
        WS_TRANS_EVENT_RECV_HEAD_OK
        | WS_TRANS_EVENT_RECV_BODY_OK
        | WS_TRANS_EVENT_BUILD_BODY
        | WS_TRANS_EVENT_DESTORY,
        _WS_PlugDWeb_EventProcess},

    {"Static",
        WS_TRANS_EVENT_RECV_HEAD_OK
        | WS_TRANS_EVENT_RECV_BODY_OK
        | WS_TRANS_EVENT_BUILD_BODY
        | WS_TRANS_EVENT_DESTORY,
        _WS_PlugStatic_EventProcess},
};

static _WS_EVENT_S g_astWsHeadFilter[] =
{
    {"ContentType", WS_TRANS_EVENT_PRE_BUILD_HEAD, _WS_PlugContentType_EventProcess},
    {"ConectionType", WS_TRANS_EVENT_PRE_BUILD_HEAD, _WS_PlugConnType_EventProcess},
};

static _WS_EVENT_TBL_S g_astWsEventTbl[] =
{
    /* 请求处理阶段 */
    {"HeadProcess", g_astWsHeadProcess, sizeof(g_astWsHeadProcess)/sizeof(_WS_EVENT_S)},
    {"Handler", g_astWsHandler, sizeof(g_astWsHandler)/sizeof(_WS_EVENT_S)},

    /* 应答阶段 */
    {"HeadFilter", g_astWsHeadFilter, sizeof(g_astWsHeadFilter)/sizeof(_WS_EVENT_S)},
};

static CHAR * ws_plug_GetPlugRetString(IN WS_EV_RET_E eEvRet)
{
    CHAR *pcString = "";

    switch (eEvRet)
    {
        case WS_EV_RET_CONTINUE:
        {
            pcString = "Continue";
            break;
        }

        case WS_EV_RET_BREAK:
        {
            pcString = "Break";
            break;
        }

        case WS_EV_RET_STOP:
        {
            pcString = "Stop";
            break;
        }

        case WS_EV_RET_INHERIT:
        {
            pcString = "Inherit";
            break;
        }

        case WS_EV_RET_ERR:
        {
            pcString = "Error";
            break;
        }

        default:
        {
            break;
        }
    }

    return pcString;
}

static WS_EV_RET_E ws_event_ProcessPlug
(
    IN WS_TRANS_S *pstTrans,
    IN UINT uiEvent,
    IN _WS_EVENT_S *pstEventPlugs,
    IN UINT uiPlugNum
)
{
    UINT i;
    WS_EV_RET_E enEvRet = WS_EV_RET_CONTINUE;
    _WS_S *pstWs;

    pstWs = pstTrans->pstWs;

    for (i=0; i<uiPlugNum; i++)
    {
        if (0 == (pstEventPlugs[i].uiCareEvents & uiEvent))
        {
            continue;
        }
        
        enEvRet = pstEventPlugs[i].pfFunc(pstTrans, uiEvent);
        _WS_DEBUG_EVENT(pstWs, ("WS_Head_Process: %s return %s.\r\n",
            pstEventPlugs[i].pcLisenserName, ws_plug_GetPlugRetString(enEvRet)));

        if (enEvRet != WS_EV_RET_CONTINUE)
        {
            if (enEvRet == WS_EV_RET_BREAK)
            {
                enEvRet = WS_EV_RET_CONTINUE;
                break;
            }
            else if (enEvRet == WS_EV_RET_STOP)
            {
                break;
            }
            else if (enEvRet == WS_EV_RET_INHERIT)
            {
                break;
            }
            else if (enEvRet == WS_EV_RET_ERR)
            {
                break;
            }
            else
            {
                BS_DBGASSERT(0);
                break;
            }
        }
    }

    return enEvRet;
}

BS_STATUS WS_Event_IssuEvent(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    _WS_S *pstWs;
    UINT i;
    WS_EV_RET_E eEvRet;
    BS_STATUS eRet = BS_OK;

    pstWs = pstTrans->pstWs;

    _WS_DEBUG_EVENT(pstWs, ("WS_Event: Issu trans event: %s.\r\n", WS_GetEventName(uiEvent)));

    for (i=0; i<sizeof(g_astWsEventTbl)/sizeof(_WS_EVENT_TBL_S); i++)
    {
        eEvRet = ws_event_ProcessPlug(pstTrans, uiEvent,
            g_astWsEventTbl[i].pstEventPlugs, g_astWsEventTbl[i].uiPlugNum);

        if ((WS_TRANS_EVENT_DESTORY != uiEvent) && ((eEvRet != WS_EV_RET_CONTINUE)))
        {
            if ((eEvRet == WS_EV_RET_STOP) || (eEvRet == WS_EV_RET_BREAK))
            {
                break;
            }
            else if (eEvRet == WS_EV_RET_INHERIT)
            {
                WS_Conn_ClearRawConn(pstTrans->hWsConn);
                eRet = BS_PROCESSED;
                break;
            }
            else if (eEvRet == WS_EV_RET_ERR)
            {
                eRet = BS_ERR;
                break;
            }
            else
            {
                BS_DBGASSERT(0);
                break;
            }
        }
    }

    return eRet;
}



