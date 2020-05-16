/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/dns_utl.h"
#include "utl/ulm_utl.h"
#include "utl/conn_utl.h"
#include "utl/ssl_utl.h"
#include "utl/http_lib.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_utl.h"
#include "../h/svpnc_func.h"

static INT svpnc_RecvCb(IN ULONG ulFd, OUT UCHAR *pucBuf, IN UINT uiBufSize)
{
    INT iLen;
    
    iLen = CONN_Read((VOID*)ulFd, pucBuf, uiBufSize);
    if (iLen > 0)
    {
        return iLen;
    }

    if (iLen == SOCKET_E_AGAIN)
    {
        return HTTPC_E_AGAIN;
    }

    if (iLen == SOCKET_E_READ_PEER_CLOSE)
    {
        return HTTPC_E_PEER_CLOSED;
    }

    return HTTPC_E_ERR;
}

BS_STATUS SVPNC_Login()
{
    CONN_HANDLE hConn;
    INT iRet;
    CHAR szString[512];

    hConn = SVPNC_SynConnectServer();
    if (NULL == hConn)
    {
        return BS_ERR;
    }

    snprintf(szString, sizeof(szString), "GET /request.cgi?_do=User.Login&UserName=%s&Password=%s HTTP/1.1\r\n\r\n",
        SVPNC_GetUserName(), SVPNC_GetUserPasswd());

    iRet = CONN_WriteString(hConn, szString);
    if (iRet < 0)
    {
        CONN_Free(hConn);
        return BS_ERR;
    }

    iRet = SVPNC_ReadHttpBody(hConn, szString, sizeof(szString) - 1);

    CONN_Free(hConn);

    if (iRet < 0)
    {
        return BS_ERR;
    }

    SVPNC_TcpRelay_Start();
    SVPNC_IpTunnel_Start();

    return BS_OK;
}


