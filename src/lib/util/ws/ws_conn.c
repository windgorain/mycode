/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-7-6
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/ws_utl.h"
    
#include "ws_def.h"
#include "ws_conn.h"
#include "ws_trans.h"

typedef struct
{
    _WS_S *pstWs;
    HANDLE hRawConn;
    PF_WS_CONN_EVENT_FUNC pfEventFunc;
    MBUF_S *pstNextReq;
    VOID *pUserData;
    USER_HANDLE_S stPrivateData;
}WS_CONN_S;

VOID WS_Conn_Destory(IN WS_CONN_HANDLE hWsConn)
{
    WS_CONN_S *pstWsConn = hWsConn;
    _WS_S *pstWs = pstWsConn->pstWs;

    pstWs->stFuncTbl.pfClose(pstWsConn);
    MEM_Free(pstWsConn);
}

BS_STATUS WS_Conn_Add(IN WS_HANDLE hWs, IN HANDLE hRawConn, IN USER_HANDLE_S *pstPrivateData)
{
    _WS_S *pstWs = hWs;
    WS_CONN_S *pstWsConn;

    pstWsConn = MEM_ZMalloc(sizeof(WS_CONN_S));
    if (NULL == pstWsConn)
    {
        return BS_ERR;
    }

    pstWsConn->pstWs = pstWs;
    pstWsConn->hRawConn = hRawConn;
    pstWsConn->pfEventFunc = _WS_Trans_EventInput;
    if (NULL != pstPrivateData)
    {
        pstWsConn->stPrivateData = *pstPrivateData;
    }

    pstWs->stFuncTbl.pfSetEvent(pstWsConn, WS_CONN_EVENT_READ);

    return BS_OK;
}

HANDLE WS_Conn_GetRawConn(IN WS_CONN_HANDLE hWsConn)
{
    WS_CONN_S *pstConn = hWsConn;

    return pstConn->hRawConn;
}

VOID WS_Conn_ClearRawConn(IN WS_CONN_HANDLE hWsConn)
{
    WS_CONN_S *pstConn = hWsConn;

    pstConn->hRawConn = NULL;
}

BS_STATUS WS_Conn_SetEvent(IN WS_CONN_HANDLE hWsConn, IN UINT uiEvent)
{
    WS_CONN_S *pstConn = hWsConn;

    return pstConn->pstWs->stFuncTbl.pfSetEvent(pstConn, uiEvent);
}

_WS_S * WS_Conn_GetWS(IN WS_CONN_HANDLE hWsConn)
{
    WS_CONN_S *pstConn = hWsConn;

    return pstConn->pstWs;
}

VOID WS_Conn_EventHandler(IN WS_CONN_HANDLE hWsConn, IN UINT uiEvent)
{
    _WS_S *pstWs;
    WS_CONN_S *pstConn = hWsConn;

    pstWs = pstConn->pstWs;

    _WS_DEBUG_EVENT(pstWs, ("WS_Conn: Recv connection event 0x%x.\r\n", uiEvent));

    if (pstConn->pfEventFunc == NULL)
    {
        WS_Conn_Destory(pstConn);
        return;
    }

    pstConn->pfEventFunc(pstConn, uiEvent);
}

INT WS_Conn_Read
(
    IN WS_CONN_HANDLE hWsConn,
    OUT UCHAR *pucBuf,
	IN UINT uiBufLen
)
{
    WS_CONN_S *pstWsConn = hWsConn;
    
    return pstWsConn->pstWs->stFuncTbl.pfRead(pstWsConn, pucBuf, uiBufLen);
}

INT WS_Conn_Write
(
    IN WS_CONN_HANDLE hWsConn,
    OUT UCHAR *pucBuf,
	IN UINT uiBufLen
)
{
    WS_CONN_S *pstWsConn = hWsConn;
    
    return pstWsConn->pstWs->stFuncTbl.pfWrite(pstWsConn, pucBuf, uiBufLen);
}

VOID WS_Conn_NextReq
(
    IN WS_CONN_HANDLE hWsConn,
    IN MBUF_S *pstMbuf
)
{
    WS_CONN_S *pstWsConn = hWsConn;

    pstWsConn->pstNextReq = pstMbuf;
}

VOID WS_Conn_SetUserData(IN WS_CONN_HANDLE hWsConn, IN VOID *pUserData)
{
    WS_CONN_S *pstWsConn = hWsConn;

    pstWsConn->pUserData = pUserData;
}

VOID * WS_Conn_GetUserData(IN WS_CONN_HANDLE hWsConn)
{
    WS_CONN_S *pstWsConn = hWsConn;

    return pstWsConn->pUserData;
}

USER_HANDLE_S * WS_Conn_GetPrivateData(IN WS_CONN_HANDLE hWsConn)
{
    WS_CONN_S *pstWsConn = hWsConn;

    return &pstWsConn->stPrivateData;
}

