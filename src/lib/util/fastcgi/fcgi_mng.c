/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-15
* Description:  FCGI管理:服务管理, 连接管理
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/fcgi_mng.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

typedef struct
{
    DLL_NODE_S stLinkNode;
    FCGIM_SERVICE_S stService;
}_FCGIM_SERVICE_NODE_S;

typedef struct
{
    DLL_HEAD_S stServiceList;   /* _FCGIM_SERVICE_NODE_S */
}_FCGIM_INSTANCE_S;

static VOID _FCGIM_FreeServiceNode(IN _FCGIM_SERVICE_NODE_S *pstNode)
{
    if (NULL != pstNode->stService.pszUrlPattern)
    {
        MEM_Free(pstNode->stService.pszUrlPattern);
    }

    if (NULL != pstNode->stService.pszFcgiAppFilePath)
    {
        MEM_Free(pstNode->stService.pszFcgiAppFilePath);
    }

    if (NULL != pstNode->stService.pszFcgiAppAddress)
    {
        MEM_Free(pstNode->stService.pszFcgiAppAddress);
    }

    MEM_Free(pstNode);
}

HANDLE FCGIM_CreateInstance()
{
    _FCGIM_INSTANCE_S *pstInstance;

    pstInstance = MEM_ZMalloc(sizeof(_FCGIM_INSTANCE_S));
    if (NULL == pstInstance)
    {
        return NULL;
    }

    DLL_INIT(&pstInstance->stServiceList);

    return pstInstance;
}

VOID FCGIM_DeleteInstance(IN HANDLE hFcgiInstance)
{
    _FCGIM_SERVICE_NODE_S *pstNode, *pstNodeTmp;
    _FCGIM_INSTANCE_S *pstInstance = hFcgiInstance;
    
    if (NULL != pstInstance)
    {
        DLL_SAFE_SCAN(&pstInstance->stServiceList, pstNode, pstNodeTmp)
        {
            DLL_DEL(&pstInstance->stServiceList, pstNode);
            _FCGIM_FreeServiceNode(pstNode);
        }
        MEM_Free(pstInstance);
    }
}

BS_STATUS FCGIM_AddService(IN HANDLE hFcgiInstance, IN FCGIM_SERVICE_S *pstService)
{
    _FCGIM_SERVICE_NODE_S *pstServiceNode;
    _FCGIM_INSTANCE_S *pstInstance = hFcgiInstance;

    pstServiceNode = MEM_ZMalloc(sizeof(_FCGIM_SERVICE_NODE_S));
    pstServiceNode->stService.usFcgiAppPort = pstService->usFcgiAppPort;

    if (pstService->pszFcgiAppFilePath != NULL)
    {
        pstServiceNode->stService.pszFcgiAppFilePath = MEM_Malloc(strlen(pstService->pszFcgiAppFilePath) + 1);
        if (pstServiceNode->stService.pszFcgiAppFilePath == NULL)
        {
            _FCGIM_FreeServiceNode(pstServiceNode);
            return (BS_NO_MEMORY);
        }
        TXT_StrCpy(pstServiceNode->stService.pszFcgiAppFilePath, pstService->pszFcgiAppFilePath);
        pstServiceNode->stService.uiSpwanNum = pstService->uiSpwanNum;
    }
    else if (pstService->pszFcgiAppAddress != NULL)
    {
        pstServiceNode->stService.pszFcgiAppAddress = MEM_Malloc(strlen(pstService->pszFcgiAppAddress) + 1);
        if (pstServiceNode->stService.pszFcgiAppAddress == NULL)
        {
            _FCGIM_FreeServiceNode(pstServiceNode);
            return (BS_NO_MEMORY);
        }
        TXT_StrCpy(pstServiceNode->stService.pszFcgiAppAddress, pstService->pszFcgiAppAddress);
        pstServiceNode->stService.usFcgiAppPort = pstService->usFcgiAppPort;
    }
    else
    {
        _FCGIM_FreeServiceNode(pstServiceNode);
        return (BS_ERR);
    }

    pstServiceNode->stService.pszUrlPattern = MEM_Malloc(strlen(pstService->pszUrlPattern) + 1);
    if (pstServiceNode->stService.pszUrlPattern == NULL)
    {
        _FCGIM_FreeServiceNode(pstServiceNode);
        return (BS_NO_MEMORY);
    }    

    TXT_StrCpy(pstServiceNode->stService.pszUrlPattern, pstService->pszUrlPattern);

    DLL_ADD(&pstInstance->stServiceList, pstServiceNode);

    return BS_OK;
}

FCGIM_SERVICE_S * FCGIM_MatchService(IN HANDLE hFcgiInstance, IN CHAR *pszUrl)
{
    _FCGIM_INSTANCE_S *pstInstance = hFcgiInstance;
    _FCGIM_SERVICE_NODE_S *pstServiceNode;
    UINT uiLen;

    DLL_SCAN(&pstInstance->stServiceList, pstServiceNode)
    {
        uiLen = strlen(pstServiceNode->stService.pszUrlPattern);

        if (pstServiceNode->stService.pszUrlPattern[0] == '/')
        {
            if (strnicmp(pstServiceNode->stService.pszUrlPattern,
                pszUrl, uiLen) == 0)
            {
                return &pstServiceNode->stService;
            }
        }
        else if (strlen(pszUrl) >= uiLen)
        {
            if (stricmp(pszUrl + strlen(pszUrl) - uiLen,
                pstServiceNode->stService.pszUrlPattern) == 0)
            {
                return &pstServiceNode->stService;
            }
        }
    }

    return NULL;
}

VOID FCGIM_Display(IN HANDLE hFcgiInstance)
{
    _FCGIM_INSTANCE_S *pstInstance = hFcgiInstance;
    _FCGIM_SERVICE_NODE_S *pstServiceNode;

    DLL_SCAN(&pstInstance->stServiceList, pstServiceNode)
    {
        if (pstServiceNode->stService.pszFcgiAppFilePath != NULL)
        {
            EXEC_OutInfo(" Path:%s AppFile:%s Spwan:%d\r\n",
                pstServiceNode->stService.pszUrlPattern,
                pstServiceNode->stService.pszFcgiAppFilePath,
                pstServiceNode->stService.uiSpwanNum);
        }
        else
        {
            EXEC_OutInfo(" Path:%s AppAddress:%s Port:%d\r\n",
                pstServiceNode->stService.pszUrlPattern,
                pstServiceNode->stService.pszFcgiAppAddress,
                pstServiceNode->stService.usFcgiAppPort);
        }
    }
}


