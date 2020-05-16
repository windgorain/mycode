/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cjson.h"
#include "utl/txt_utl.h"
#include "utl/conn_utl.h"
#include "utl/ssl_utl.h"
#include "utl/mime_utl.h"
#include "utl/http_lib.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_utl.h"
#include "../h/svpnc_func.h"

#include "svpnc_tcprelay_inner.h"

static CHAR * svpnc_tcprelay_GetJsonValueByName(IN cJSON *pstJson, IN CHAR *pcName)
{
    cJSON *pstNode;

    pstNode = cJSON_GetObjectItem(pstJson, pcName);
    if ((NULL == pstNode) || (pstNode->valuestring == NULL))
    {
        return "";
    }

    return pstNode->valuestring;
}

static VOID svpnc_tcprelay_ProcessTcpJsonNode(IN cJSON *pstTcpRelayNode)
{
    CHAR *pcName, *pcServerAdr;

    pcName = svpnc_tcprelay_GetJsonValueByName(pstTcpRelayNode, "Name");
    pcServerAdr = svpnc_tcprelay_GetJsonValueByName(pstTcpRelayNode, "ServerAddress");

    SVPNC_TrRes_Add(pcName, pcServerAdr);

    return;
}

static VOID svpnc_tcprelay_ParseJsonString(IN CHAR *pcString)
{
    cJSON *pstJson;
    cJSON *pstJsonData;
    cJSON *pstTcpRelayNode;
    INT iSize;
    INT i;

    pstJson = cJSON_Parse(pcString);
    if (NULL == pstJson)
    {
        return;
    }

    pstJsonData = cJSON_GetObjectItem(pstJson, "Data");
    if (NULL != pstJsonData)
    {
        iSize = cJSON_GetArraySize(pstJsonData);
        for (i=0; i<iSize; i++)
        {
            pstTcpRelayNode = cJSON_GetArrayItem(pstJsonData, i);
            svpnc_tcprelay_ProcessTcpJsonNode(pstTcpRelayNode);
        }
    }

    cJSON_Delete(pstJson);
}

BS_STATUS SVPNC_TcpRelay_Start()
{
    /* 获取TcpRelay的配置 */
    CONN_HANDLE hConn;
    CHAR szString[4096];
    INT iLen;

    hConn = SVPNC_SynConnectServer();
    if (hConn == NULL)
    {
        return BS_ERR;
    }

    snprintf(szString, sizeof(szString),
        "GET /request.cgi?_do=TcpRes.List HTTP/1.1\r\n"
        "Cookie: svpnuid=%s\r\n\r\n", SVPNC_GetCookie());

    if (CONN_WriteString(hConn, szString) < 0)
    {
        CONN_Free(hConn);
        return BS_ERR;
    }

    iLen = SVPNC_ReadHttpBody(hConn, szString, sizeof(szString)-1);
    CONN_Free(hConn);
    if (iLen < 0)
    {
        return BS_ERR;
    }

    szString[iLen] = '\0';

    svpnc_tcprelay_ParseJsonString(szString);

    /* 触发写文件 */
    SVPNC_TR_Write2File();

    return BS_OK;
}


