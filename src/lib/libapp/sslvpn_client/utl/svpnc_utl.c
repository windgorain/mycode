/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-1
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/conn_utl.h"
#include "utl/ssl_utl.h"
#include "utl/mime_utl.h"
#include "utl/http_lib.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_utl.h"

static VOID * svpnc_CreateSsl(IN INT iSocketID)
{
    VOID *pstSsl;

    pstSsl = SSL_UTL_New(SVPNC_GetSslCtx());
    if (NULL == pstSsl)
    {
        return NULL;
    }

    SSL_UTL_SetFd(pstSsl, iSocketID);
    SSL_UTL_SetHostName(pstSsl, SVPNC_GetServer());

    return pstSsl;
}

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

CONN_HANDLE SVPNC_CreateServerConn()
{
    INT iSocketID;
    VOID *pSsl = NULL;
    CONN_HANDLE hConn;

    iSocketID = Socket_Create(AF_INET, SOCK_STREAM);
    if (iSocketID < 0)
    {
        return NULL;
    }

    Socket_SetNoDelay(iSocketID, TRUE);

    if (SVPNC_GetConnType() == SVPNC_CONN_TYPE_SSL)
    {
        pSsl = svpnc_CreateSsl(iSocketID);
        if (NULL == pSsl)
        {
            Socket_Close(iSocketID);
            return NULL;
        }
    }

    hConn = CONN_New(iSocketID);
    if (NULL == hConn)
    {
        if (pSsl != NULL)
        {
            SSL_UTL_Free(pSsl);
        }
        Socket_Close(iSocketID);
        return NULL;
    }

    CONN_SetSsl(hConn, pSsl);

    return hConn;
}

CONN_HANDLE SVPNC_SynConnectServer()
{
    CONN_HANDLE hConn;

    hConn = SVPNC_CreateServerConn();

    if (BS_OK != Socket_Connect(CONN_GetFD(hConn), SVPNC_GetServerIP(), SVPNC_GetServerPort()))
    {
        CONN_Free(hConn);
        return NULL;
    }

    if (CONN_GetSsl(hConn) != NULL)
    {
        if (SSL_UTL_Connect(CONN_GetSsl(hConn))< 0)
        {
            CONN_Free(hConn);
            return NULL;
        }
    }

    return hConn;
}

static VOID svpnc_ProcessSetCookie(IN HTTPC_RECVER_HANDLE hRecver)
{
    HTTP_HEAD_PARSER hParser;
    CHAR *pcSetCookie;
    MIME_HANDLE hMime;
    CHAR *pcSvpnUid;
    
    hParser = HttpcRecver_GetHttpParser(hRecver);
    if (NULL == hParser)
    {
        return;
    }

    hMime = MIME_Create();
    if (NULL == hMime)
    {
        return;
    }

    pcSetCookie = HTTP_GetHeadField(hParser, HTTP_FIELD_SET_COOKIE);
    if ((NULL != pcSetCookie) && (BS_OK == MIME_ParseCookie(hMime, pcSetCookie)))
    {
        pcSvpnUid = MIME_GetKeyValue(hMime, "svpnuid");
        if (NULL != pcSvpnUid)
        {
            SVPNC_SetCookie(pcSvpnUid);
        }
    }
    
    return;
}

INT SVPNC_ReadHttpBody(IN CONN_HANDLE hConn, IN UCHAR *pucData, IN UINT uiDataSize)
{
    INT iRet = 0;
    HTTPC_RECVER_HANDLE hRecver;
    HTTPC_RECVER_PARAM_S stParam;
    CHAR *pcTmp;
    UINT uiLen;
    INT iReadLen = 0;
    

    stParam.ulFd = (ULONG)hConn;
    stParam.pfRecv = svpnc_RecvCb;
    
    hRecver = HttpcRecver_Create(&stParam);
    if (NULL == hRecver)
    {
        return BS_ERR;
    }

    pcTmp = pucData;
    uiLen = uiDataSize;

    while (uiLen > 0)
    {
        iRet = HttpcRecver_Read(hRecver, pcTmp, uiLen);
        if (iRet < 0)
        {
            if (iRet == HTTPC_E_AGAIN)
            {
                continue;
            }
            else

            {
                break;
            }
        }

        pcTmp += iRet;
        uiLen -= iRet;
        iReadLen += iRet;
    }

    svpnc_ProcessSetCookie(hRecver);

    HttpcRecver_Destroy(hRecver);

    if (iRet != HTTPC_E_FINISH)
    {
        return HTTPC_E_ERR;
    }

    return iReadLen;
}

