/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-5-24
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/ws_utl.h"
#include "utl/name_bit.h"

#include "ws_def.h"
#include "ws_conn.h"
#include "ws_trans.h"
#include "ws_drp.h"

static NAME_BIT_S g_astWsDbgNameBits[] =
{
    {"packet", WS_DBG_PACKET},
    {"event", WS_DBG_EVENT},
    {"process", WS_DBG_PROCESS},
    {"err", WS_DBG_ERR},
    {"all", WS_DBG_ALL},

    {NULL, 0},
};


WS_HANDLE WS_Create(IN WS_FUNC_TBL_S *pstFuncTbl)
{
    _WS_S *pstWs;

    if (NULL == pstFuncTbl)
    {
        return NULL;
    }

    _WS_Trans_Init();
    _WS_DRP_Init();

    pstWs = MEM_ZMalloc(sizeof(_WS_S));
    if (NULL == pstWs)
    {
        return NULL;
    }

    pstWs->stFuncTbl = *pstFuncTbl;

    DLL_INIT(&pstWs->stVHostList);

    return pstWs;
}

CHAR * WS_GetEventName(IN UINT uiEvent)
{
    CHAR *pcName = "";
    
    switch (uiEvent)
    {
        case WS_TRANS_EVENT_CREATE:
        {
            pcName = "Create";
            break;
        }

        case WS_TRANS_EVENT_RECV_HEAD_OK:
        {
            pcName = "RecvHeadOK";
            break;
        }

        case WS_TRANS_EVENT_RECV_BODY:
        {
            pcName = "RecvBody";
            break;
        }

        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            pcName = "RecvBodyOK";
            break;
        }

        case WS_TRANS_EVENT_PRE_BUILD_HEAD:
        {
            pcName = "PreBuildHead";
            break;
        }

        case WS_TRANS_EVENT_SEND_HEAD_OK:
        {
            pcName = "SendHeadOK";
            break;
        }

        case WS_TRANS_EVENT_BUILD_BODY:
        {
            pcName = "BuildBody";
            break;
        }

        case WS_TRANS_EVENT_FORMAT_BODY:
        {
            pcName = "FormatBody";
            break;
        }

        case WS_TRANS_EVENT_SEND_BODY_OK:
        {
            pcName = "SendBodyOK";
            break;
        }

        case WS_TRANS_EVENT_DESTORY:
        {
            pcName = "Destory";
            break;
        }

        default:
        {
            break;
        }
    }

    return pcName;
}



UINT WS_GetDbgFlagByName(IN CHAR *pcFlagName)
{
    return NameBit_GetBitByName(g_astWsDbgNameBits, pcFlagName);
}

VOID WS_SetDbg(IN WS_HANDLE hWs, IN UINT uiDbgFlag)
{
    _WS_S *pstWs = hWs;

    pstWs->uiDbgFlag |= uiDbgFlag;
}

VOID WS_ClrDbg(IN WS_HANDLE hWs, IN UINT uiDbgFlag)
{
    _WS_S *pstWs = hWs;

    BIT_CLR(pstWs->uiDbgFlag, uiDbgFlag);
}

VOID WS_SetDbgFlagByName(IN WS_HANDLE hWs, IN CHAR *pcFlagName)
{
    UINT uiFlag = WS_GetDbgFlagByName(pcFlagName);

    if (uiFlag != 0)
    {
        WS_SetDbg(hWs, uiFlag);
    }
}

VOID WS_ClrDbgFlagByName(IN WS_HANDLE hWs, IN CHAR *pcFlagName)
{
    UINT uiFlag = WS_GetDbgFlagByName(pcFlagName);

    if (uiFlag != 0)
    {
        WS_ClrDbg(hWs, uiFlag);
    }
}

