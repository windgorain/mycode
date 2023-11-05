/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-7-26
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/txt_utl.h"
#include "utl/ws_utl.h"

#include "ws_def.h"
#include "ws_context.h"
#include "ws_vhost.h"

static _WS_VHOST_S * ws_vhost_Find(IN _WS_S *pstWs, IN CHAR *pcVHost)
{
    _WS_VHOST_S *pstVHost;
    UINT uiVHostLen;

    if (NULL == pcVHost)
    {
        pcVHost = "";
    }

    uiVHostLen = strlen(pcVHost);

    DLL_SCAN(&pstWs->stVHostList, pstVHost)
    {
        if ((pstVHost->uiVHostLen == uiVHostLen)
            && (strcmp(pcVHost, pstVHost->szVHost) == 0))
        {
            return pstVHost;
        }
    }

    return NULL;
}

static _WS_VHOST_S * ws_vhost_Add(IN _WS_S *pstWs, IN CHAR *pcVHost)
{
    _WS_VHOST_S *pstVHost;

    pstVHost = MEM_ZMalloc(sizeof(_WS_VHOST_S));
    if (NULL == pstVHost)
    {
        return NULL;
    }

    TXT_Strlcpy(pstVHost->szVHost, pcVHost, sizeof(pstVHost->szVHost));
    pstVHost->uiVHostLen = strlen(pstVHost->szVHost);
    pstVHost->pstWs = pstWs;
    _WS_Context_InitContainer(pstVHost);

    DLL_ADD(&pstWs->stVHostList, pstVHost);

    return pstVHost;
}

static _WS_VHOST_S * ws_vhost_Match(IN _WS_S *pstWs, IN CHAR *pcVHost, IN UINT uiVHostLen)
{
    _WS_VHOST_S *pstVHost;
    _WS_VHOST_S *pstVHostFound = NULL;

    DLL_SCAN(&pstWs->stVHostList, pstVHost)
    {
        if (pstVHost->uiVHostLen == 0)
        {
            pstVHostFound = pstVHost;
        }

        if ((pstVHost->uiVHostLen == uiVHostLen)
            && (strnicmp(pcVHost, pstVHost->szVHost, uiVHostLen) == 0))
        {
            pstVHostFound = pstVHost;
            break;
        }
    }

    return pstVHostFound;
}

WS_VHOST_HANDLE WS_VHost_Add(IN WS_HANDLE hWs, IN CHAR *pcVHost)
{
    _WS_VHOST_S *pstVHost;

    if (NULL == pcVHost)
    {
        pcVHost = "";
    }

    if (strlen(pcVHost) > WS_VHOST_MAX_LEN)
    {
        return NULL;
    }

    pstVHost = ws_vhost_Find(hWs, pcVHost);
    if (NULL != pstVHost)
    {
        return NULL;
    }

    return ws_vhost_Add(hWs, pcVHost);
}

VOID WS_VHost_Del(IN WS_VHOST_HANDLE hVHost)
{
    _WS_VHOST_S *pstVHost = hVHost;

    if (NULL == pstVHost)
    {
        return;
    }

    DLL_DEL(&pstVHost->pstWs->stVHostList, pstVHost);

    WS_Context_DelAll(pstVHost);

    MEM_Free(pstVHost);
}


WS_VHOST_HANDLE WS_VHost_Find(IN WS_HANDLE hWs, IN CHAR *pcVHost)
{
    return ws_vhost_Find(hWs, pcVHost);
}


WS_VHOST_HANDLE WS_VHost_Match(IN WS_HANDLE hWs, IN CHAR *pcVHost, IN UINT uiVHostLen)
{
    if (NULL == pcVHost)
    {
        pcVHost = "";
        uiVHostLen = 0;
    }

    return ws_vhost_Match(hWs, pcVHost, uiVHostLen);
}

UINT WS_VHost_GetContextCount(IN WS_VHOST_HANDLE hVHost)
{
    _WS_VHOST_S *pstVHost = hVHost;

    if (NULL == pstVHost)
    {
        return 0;
    }

    return DLL_COUNT(&pstVHost->stContexts.stContextList);
}


