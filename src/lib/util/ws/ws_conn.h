/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-6
* Description: 
* History:     
******************************************************************************/

#ifndef __WS_CONN_H_
#define __WS_CONN_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef BS_STATUS (*PF_WS_CONN_EVENT_FUNC)(IN WS_CONN_HANDLE hWsConn, IN UINT uiEvent);

_WS_S * WS_Conn_GetWS(IN WS_CONN_HANDLE hWsConn);

INT WS_Conn_Read
(
    IN WS_CONN_HANDLE hWsConn,
    OUT UCHAR *pucBuf,
	IN UINT uiBufLen
);
INT WS_Conn_Write
(
    IN WS_CONN_HANDLE hWsConn,
    OUT UCHAR *pucBuf,
	IN UINT uiBufLen
);

VOID WS_Conn_NextReq
(
    IN WS_CONN_HANDLE hWsConn,
    OUT MBUF_S *pstMbuf
);

VOID WS_Conn_SetUserData(IN WS_CONN_HANDLE hWsConn, IN VOID *pUserData);

VOID * WS_Conn_GetUserData(IN WS_CONN_HANDLE hWsConn);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WS_CONN_H_*/


