/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-27
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/ws_utl.h"

#include "../ws_def.h"
#include "../ws_conn.h"
#include "../ws_trans.h"
#include "../ws_event.h"
#include "../ws_context.h"
#include "../ws_deliver.h"

#define _WS_PLUG_DELIVER_MAX_PARSE_BODY_LEN 2048

typedef struct
{
    _WS_DELIVER_NODE_S *pstDlvInfo;
    PF_WS_Deliver_Func pfDeliverFunc;
    MBUF_S *pstMbuf;
}_WS_PLUG_DELIVER_NODE_S;

static inline WS_EV_RET_E ws_plugdevliver_DeliverRet2EvRet(IN WS_DELIVER_RET_E eDeliverRet)
{
    WS_EV_RET_E eRet;
    
    switch (eDeliverRet)
    {
        case WS_DELIVER_RET_OK:
        {
            eRet = WS_EV_RET_BREAK;
            break;
        }

        case WS_DELIVER_RET_INHERIT:
        {
            eRet = WS_EV_RET_INHERIT;
            break;
        }

        case WS_DELIVER_RET_ERR:
        {
            eRet = WS_EV_RET_ERR;
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            eRet = WS_EV_RET_ERR;
            break;
        }
    }

    return eRet;
}

static WS_EV_RET_E ws_plugdeliver_HeadOK(IN WS_TRANS_S *pstTrans)
{
    _WS_DELIVER_NODE_S *pstDlvInfo;
    _WS_PLUG_DELIVER_NODE_S *pstDlvNode;
    WS_EV_RET_E eRet;
    WS_DELIVER_RET_E eDeliverRet;

    pstDlvInfo = _WS_Deliver_Match(pstTrans);
    if (NULL == pstDlvInfo)
    {
        return WS_EV_RET_CONTINUE;
    }

    pstDlvNode = MEM_ZMalloc(sizeof(_WS_PLUG_DELIVER_NODE_S));
    if (NULL == pstDlvNode)
    {
        return WS_EV_RET_ERR;
    }

    pstDlvNode->pstDlvInfo = pstDlvInfo;
    pstDlvNode->pfDeliverFunc = pstDlvInfo->pfFunc;

    pstTrans->apPlugContext[WS_PLUG_DELIVER] = pstDlvNode;

    eRet = WS_EV_RET_BREAK;
    if (pstDlvInfo->uiFlag & WS_DELIVER_FLAG_DELIVER_BODY)
    {
        eDeliverRet = pstDlvNode->pfDeliverFunc(pstTrans, WS_TRANS_EVENT_RECV_HEAD_OK);
        eRet = ws_plugdevliver_DeliverRet2EvRet(eDeliverRet);
    }

    return eRet;
}

static WS_EV_RET_E ws_plugdeliver_RecvBody(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_DELIVER_NODE_S *pstDlvNode;
    _WS_DELIVER_NODE_S *pstDlvInfo;
    MBUF_S *pstMbuf;
    WS_EV_RET_E eRet;
    WS_DELIVER_RET_E eDeliverRet;

    pstDlvNode = pstTrans->apPlugContext[WS_PLUG_DELIVER];
    if (NULL == pstDlvNode)
    {
        return WS_EV_RET_CONTINUE;
    }

    pstDlvInfo = pstDlvNode->pstDlvInfo;

    eRet = WS_EV_RET_BREAK;

    if (pstDlvInfo->uiFlag & WS_DELIVER_FLAG_DROP_BODY)
    {
        pstMbuf = WS_Trans_GetBodyData(pstTrans);
        if (NULL != pstMbuf)
        {
            MBUF_Free(pstMbuf);
        }
    }
    else if (pstDlvInfo->uiFlag & WS_DELIVER_FLAG_DELIVER_BODY)
    {
        eDeliverRet = pstDlvNode->pfDeliverFunc(pstTrans, WS_TRANS_EVENT_RECV_BODY);
        eRet = ws_plugdevliver_DeliverRet2EvRet(eDeliverRet);
    }
    else if (pstDlvInfo->uiFlag & WS_DELIVER_FLAG_PARSE_BODY_AS_MIME)
    {
        pstMbuf = WS_Trans_GetBodyData(pstTrans);
        if (NULL != pstMbuf)
        {
            MBUF_CAT_EXT(pstDlvNode->pstMbuf, pstMbuf);
            if (MBUF_TOTAL_DATA_LEN(pstDlvNode->pstMbuf) > _WS_PLUG_DELIVER_MAX_PARSE_BODY_LEN)
            {
                return WS_EV_RET_ERR;
            }
        }
    }

    return eRet;
}

static WS_EV_RET_E ws_plugdeliver_RecvBodyOK(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_DELIVER_NODE_S *pstDlvNode;
    MBUF_S *pstMbuf;
    _WS_DELIVER_NODE_S *pstDlvInfo;
    CHAR *pcData;
    HANDLE hMemHandle;
    WS_DELIVER_RET_E eDeliverRet;

    pstDlvNode = pstTrans->apPlugContext[WS_PLUG_DELIVER];
    if (NULL == pstDlvNode)
    {
        return WS_EV_RET_CONTINUE;
    }

    pstDlvInfo = pstDlvNode->pstDlvInfo;

    if (pstDlvInfo->uiFlag & WS_DELIVER_FLAG_PARSE_BODY_AS_MIME)
    {
        pstMbuf = pstDlvNode->pstMbuf;
        if (NULL != pstMbuf)
        {
            pcData = MBUF_GetContinueMemRaw(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), &hMemHandle);
            if (NULL == pcData)
            {
                return WS_EV_RET_ERR;
            }

            pstTrans->hBodyMime = MIME_Create();
            if (NULL == pstTrans->hBodyMime)
            {
                MBUF_FreeContinueMem(hMemHandle);
                return WS_EV_RET_ERR;
            }

            MIME_ParseData(pstTrans->hBodyMime, pcData);

            MBUF_FreeContinueMem(hMemHandle);
        }
    }

    eDeliverRet = pstDlvNode->pfDeliverFunc(pstTrans, WS_TRANS_EVENT_RECV_BODY_OK);

    return ws_plugdevliver_DeliverRet2EvRet(eDeliverRet);
}

static WS_EV_RET_E ws_plugdeliver_Destory(IN WS_TRANS_S *pstTrans)
{
    _WS_PLUG_DELIVER_NODE_S *pstDlvNode;

    pstDlvNode = pstTrans->apPlugContext[WS_PLUG_DELIVER];
    if (NULL == pstDlvNode)
    {
        return WS_EV_RET_CONTINUE;
    }

    pstTrans->apPlugContext[WS_PLUG_DELIVER] = NULL;

    if (pstDlvNode->pstDlvInfo != NULL)
    {
        pstDlvNode->pfDeliverFunc(pstTrans, WS_TRANS_EVENT_DESTORY);
    }

    if (NULL != pstDlvNode->pstMbuf)
    {
        MBUF_Free(pstDlvNode->pstMbuf);
        pstDlvNode->pstMbuf = NULL;
    }

    MEM_Free(pstDlvNode);

    return WS_EV_RET_CONTINUE;
}

static WS_EV_RET_E ws_plugdeliver_ProcessCommonEvent(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    _WS_PLUG_DELIVER_NODE_S *pstDlvNode;
    WS_DELIVER_RET_E eDeliverRet;

    pstDlvNode = pstTrans->apPlugContext[WS_PLUG_DELIVER];
    if (NULL == pstDlvNode)
    {
        return WS_EV_RET_CONTINUE;
    }

    eDeliverRet = pstDlvNode->pfDeliverFunc(pstTrans, uiEvent);

    return ws_plugdevliver_DeliverRet2EvRet(eDeliverRet);
}

WS_EV_RET_E _WS_PlugDeliver_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    WS_EV_RET_E eRet = WS_EV_RET_CONTINUE;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_HEAD_OK:
        {
            eRet = ws_plugdeliver_HeadOK(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_RECV_BODY:
        {
            eRet = ws_plugdeliver_RecvBody(pstTrans);
            break;
        }
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = ws_plugdeliver_RecvBodyOK(pstTrans);
            break;
        }

        case WS_TRANS_EVENT_BUILD_BODY:
        {
            eRet = ws_plugdeliver_ProcessCommonEvent(pstTrans, uiEvent);
            break;
        }

        case WS_TRANS_EVENT_DESTORY:
        {
            ws_plugdeliver_Destory(pstTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}

BS_STATUS WS_Deliver_SetDeliverFunc(IN WS_TRANS_HANDLE hWsTrans, IN PF_WS_Deliver_Func pfDeliverFunc)
{
    WS_TRANS_S *pstTrans = hWsTrans;
    _WS_PLUG_DELIVER_NODE_S *pstDlvNode;

    if (NULL == pstTrans)
    {
        BS_DBGASSERT(0);
        return BS_ERR;
    }

    pstDlvNode = pstTrans->apPlugContext[WS_PLUG_DELIVER];
    if (NULL == pstDlvNode)
    {
        return BS_NOT_FOUND;
    }

    pstDlvNode->pfDeliverFunc = pfDeliverFunc;

    return BS_OK;
}

