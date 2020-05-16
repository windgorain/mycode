/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-7-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/ws_utl.h"

#include "ws_def.h"
#include "ws_context.h"
#include "ws_vhost.h"
#include "ws_trans.h"
#include "ws_deliver.h"

static _WS_CONTEXT_S * ws_context_Find(IN _WS_VHOST_S *pstVHost, IN CHAR *pcDomain)
{
    _WS_CONTEXT_S *pstContext;
    UINT uiDomainLen;

    uiDomainLen = strlen(pcDomain);

    DLL_SCAN(&pstVHost->stContexts.stContextList, pstContext)
    {
        if ((pstContext->uiDomainLen == uiDomainLen)
            && (strcmp(pcDomain, pstContext->szDomain) == 0))
        {
            return pstContext;
        }
    }

    return NULL;
}

static _WS_CONTEXT_S * ws_context_Add(IN _WS_VHOST_S *pstVHost, IN CHAR *pcDomain)
{
    _WS_CONTEXT_S *pstContext;

    pstContext = MEM_ZMalloc(sizeof(_WS_CONTEXT_S));
    if (NULL == pstContext)
    {
        return NULL;
    }

    TXT_Strlcpy(pstContext->szDomain, pcDomain, sizeof(pstContext->szDomain));
    pstContext->uiDomainLen = strlen(pstContext->szDomain);
    pstContext->pVHost = pstVHost;

    DLL_ADD(&pstVHost->stContexts.stContextList, pstContext);

    return pstContext;
}

BS_STATUS _WS_Context_InitContainer(INOUT _WS_VHOST_S *pstVHost)
{
    DLL_INIT(&pstVHost->stContexts.stContextList);
    pstVHost->stContexts.stDftContext.pVHost = pstVHost;

    return BS_OK;
}

WS_CONTEXT_HANDLE _WS_Context_Match(IN WS_VHOST_HANDLE hVHost, IN CHAR *pcContext)
{
    _WS_VHOST_S *pstVHost = hVHost;

    /* 除了Dft context外只有一个Context, 则可以直接跳过选择过程 */
    if (DLL_COUNT(&pstVHost->stContexts.stContextList) == 1)
    {
        return DLL_FIRST(&pstVHost->stContexts.stContextList);
    }

    return WS_Context_Find(hVHost, pcContext);
}

WS_CONTEXT_HANDLE WS_Context_Add
(
    IN WS_VHOST_HANDLE hVHost,
    IN CHAR *pcDomain
)
{
    _WS_VHOST_S *pstVHost = hVHost;
    _WS_CONTEXT_S *pstContext;

    if ((NULL == pcDomain) || (pcDomain[0] == '\0'))
    {
        pcDomain = "[Anonymous]";
    }

    if (strlen(pcDomain) > WS_DOMAIN_MAX_LEN)
    {
        return NULL;
    }

    pstContext = ws_context_Find(pstVHost, pcDomain);
    if (NULL != pstContext)
    {
        return NULL;  /* 冲突了 */
    }

    return ws_context_Add(pstVHost, pcDomain);
}

WS_CONTEXT_HANDLE WS_Context_Find(IN WS_VHOST_HANDLE hVHost, IN CHAR *pcContext)
{
    _WS_VHOST_S *pstVHost = hVHost;

    if (NULL == pcContext)
    {
        pcContext = "";
    }

    return ws_context_Find(pstVHost, pcContext);
}

WS_CONTEXT_HANDLE WS_Context_GetDftContext(IN WS_VHOST_HANDLE hVHost)
{
    _WS_VHOST_S *pstVHost = hVHost;

    return &pstVHost->stContexts.stDftContext;
}

VOID WS_Context_Del(IN WS_CONTEXT_HANDLE hWsContext)
{
    _WS_VHOST_S *pstVHost;
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext)
    {
        return;
    }

    pstContext = hWsContext;
    pstVHost = pstContext->pVHost;

    if (&pstVHost->stContexts.stDftContext == pstContext)
    {
        return;
    }

    DLL_DEL(&pstVHost->stContexts.stContextList, pstContext);
    MEM_Free(pstContext);
}

VOID WS_Context_DelAll(IN WS_VHOST_HANDLE hVHost)
{
    _WS_VHOST_S *pstVHost = hVHost;
    _WS_CONTEXT_S *pstContext;
    _WS_CONTEXT_S *pstContextTmp;

    DLL_SAFE_SCAN(&pstVHost->stContexts.stContextList, pstContext, pstContextTmp)
    {
        DLL_DEL(&pstVHost->stContexts.stContextList, pstContext);
        MEM_Free(pstContext);
    }
}

BS_STATUS WS_Context_SetRootPath(IN WS_CONTEXT_HANDLE hWsContext, IN CHAR *pcPath)
{
    _WS_CONTEXT_S *pstContext;

    if ((NULL == hWsContext) || (NULL == pcPath))
    {
        return BS_NULL_PARA;
    }

    if (strlen(pcPath) > FILE_MAX_PATH_LEN)
    {
        return BS_OUT_OF_RANGE;
    }

    pstContext = hWsContext;

    TXT_Strlcpy(pstContext->szRootPath, pcPath, sizeof(pstContext->szRootPath));

    return BS_OK;
}

BS_STATUS WS_Context_SetSecRootPath(IN WS_CONTEXT_HANDLE hWsContext, IN CHAR *pcPath)
{
    _WS_CONTEXT_S *pstContext;

    if ((NULL == hWsContext) || (NULL == pcPath))
    {
        return BS_NULL_PARA;
    }

    if (strlen(pcPath) > FILE_MAX_PATH_LEN)
    {
        return BS_OUT_OF_RANGE;
    }

    pstContext = hWsContext;

    TXT_Strlcpy(pstContext->szSecondRootPath, pcPath, sizeof(pstContext->szSecondRootPath));

    return BS_OK;
}

CHAR * WS_Context_GetRootPath(IN WS_CONTEXT_HANDLE hWsContext)
{
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext) 
    {
        return NULL;
    }

    pstContext = hWsContext;

    return pstContext->szRootPath;
}

CHAR * WS_Context_GetSecRootPath(IN WS_CONTEXT_HANDLE hWsContext)
{
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext) 
    {
        return NULL;
    }

    pstContext = hWsContext;

    return pstContext->szSecondRootPath;
}

static CHAR * ws_context_File2RootPathFile
(
    IN CHAR *pcRootPath,
    IN CHAR *pcFilePath,
    OUT CHAR *pcRootPathFile,
    IN UINT uiRootPathFileSize
)
{
    UINT uiCpyLen;
    UINT uiSize;

    if ((pcRootPath == NULL) || (pcRootPath[0] == '\0'))
    {
        return NULL;
    }

    uiCpyLen = TXT_Strlcpy(pcRootPathFile, pcRootPath, uiRootPathFileSize);
    if (uiCpyLen >= uiRootPathFileSize)
    {
        return NULL;
    }

    uiSize = uiRootPathFileSize - uiCpyLen;
    uiCpyLen = TXT_Strlcpy(pcRootPathFile + uiCpyLen, pcFilePath, uiRootPathFileSize - uiCpyLen);
    if (uiCpyLen >= uiSize)
    {
        return NULL;
    }

    if (TRUE != FILE_IsFileExist(pcRootPathFile))
    {    
        return NULL;
    }

    return pcRootPathFile;
}

CHAR * WS_Context_File2RootPathFile
(
    IN WS_CONTEXT_HANDLE hWsContext,
    IN CHAR *pcFilePath,
    OUT CHAR *pcRootPathFile,
    IN UINT uiRootPathFileSize
)
{
    CHAR *pcRootFilePathTmp = NULL;

    pcRootFilePathTmp = ws_context_File2RootPathFile(WS_Context_GetRootPath(hWsContext),
        pcFilePath, pcRootPathFile, uiRootPathFileSize);
    if (NULL != pcRootFilePathTmp)
    {
        return pcRootFilePathTmp;
    }

    pcRootFilePathTmp = ws_context_File2RootPathFile(WS_Context_GetSecRootPath(hWsContext),
        pcFilePath, pcRootPathFile, uiRootPathFileSize);
    if (NULL != pcRootFilePathTmp)
    {
        return pcRootFilePathTmp;
    }

    return NULL;
}

BS_STATUS WS_Context_SetIndex(IN WS_CONTEXT_HANDLE hWsContext, IN CHAR *pcIndex)
{
    _WS_CONTEXT_S *pstContext;

    if ((NULL == hWsContext) || (NULL == pcIndex))
    {
        return BS_NULL_PARA;
    }

    if (strlen(pcIndex) > WS_CONTEXT_MAX_INDEX_LEN)
    {
        return BS_OUT_OF_RANGE;
    }

    pstContext = hWsContext;

    TXT_Strlcpy(pstContext->szIndex, pcIndex, sizeof(pstContext->szIndex));

    return BS_OK;
}

CHAR * WS_Context_GetIndex(IN WS_CONTEXT_HANDLE hWsContext)
{
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext) 
    {
        return NULL;
    }

    pstContext = hWsContext;

    return pstContext->szIndex;
}

WS_VHOST_HANDLE WS_Context_GetVHost(IN WS_CONTEXT_HANDLE hWsContext)
{
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext) 
    {
        return NULL;
    }

    pstContext = hWsContext;

    return pstContext->pVHost;
}

CHAR * WS_Context_GetDomainName(IN WS_CONTEXT_HANDLE hWsContext)
{
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext) 
    {
        return NULL;
    }

    pstContext = hWsContext;

    return pstContext->szDomain;
}

CHAR * WS_Context_GetNext(IN WS_VHOST_HANDLE hVHost, IN CHAR *pcCurrentContext)
{
    _WS_VHOST_S *pstVHost = hVHost;
    _WS_CONTEXT_S *pstContext;
    CHAR *pcFind = NULL;

    if (NULL == pcCurrentContext)
    {
        pcCurrentContext = "";
    }

    DLL_SCAN(&pstVHost->stContexts.stContextList, pstContext)
    {
        if (strcmp(pcCurrentContext, pstContext->szDomain) < 0)
        {
            if ((pcFind == NULL) || (strcmp(pstContext->szDomain, pcFind) < 0))
            {
                pcFind = pstContext->szDomain;
            }
        }
    }

    return pcFind;
}

VOID WS_Context_BindDeliverTbl(IN WS_CONTEXT_HANDLE hWsContext, IN WS_DELIVER_TBL_HANDLE hDeliverTbl)
{
    _WS_CONTEXT_S *pstContext = hWsContext;

    if (NULL == hWsContext) 
    {
        return;
    }

    pstContext->hDeliverTbl = hDeliverTbl;
}

BS_STATUS WS_Context_SetUserData(IN WS_CONTEXT_HANDLE hWsContext, IN VOID *pUserData)
{
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext) 
    {
        return BS_NULL_PARA;
    }

    pstContext = hWsContext;

    pstContext->pUserData = pUserData;

    return BS_OK;
}

VOID * WS_Context_GetUserData(IN WS_CONTEXT_HANDLE hWsContext)
{
    _WS_CONTEXT_S *pstContext;

    if (NULL == hWsContext) 
    {
        return NULL;
    }

    pstContext = hWsContext;

    return pstContext->pUserData;
}

