/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-1
* Description: Key-Value Func Tbl. 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/kf_utl.h"

typedef struct
{
    DLL_NODE_S stNode;
    CHAR *pcKey;
    PF_KF_FUNC pfFunc;
    HANDLE hUserHandle;
}_KF_NODE_S;

typedef struct
{
    DLL_HEAD_S stList;
    CHAR *pcActionString;
}_KF_CTRL_S;

static _KF_NODE_S * mf_Find(IN _KF_CTRL_S *pstCtrl, IN CHAR *pcKey)
{
    _KF_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stList, pstNode)
    {
        if (strcmp(pcKey, pstNode->pcKey) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}

KF_HANDLE KF_Create(IN CHAR *pcActionString)
{
    _KF_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_KF_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    DLL_INIT(&pstCtrl->stList);
    pstCtrl->pcActionString = pcActionString;

    return pstCtrl;
}

VOID KF_Destory(IN KF_HANDLE hKf)
{
    _KF_CTRL_S *pstCtrl;
    _KF_NODE_S *pstNode;
    _KF_NODE_S *pstNodeTmp;

    pstCtrl = hKf;

    if (NULL == pstCtrl)
    {
        return;
    }

    DLL_SAFE_SCAN(&pstCtrl->stList, pstNode, pstNodeTmp)
    {
        MEM_Free(pstNode);
    }

    MEM_Free(pstCtrl);
}

BS_STATUS KF_AddFunc(IN KF_HANDLE hKf, IN CHAR *pcKey, IN PF_KF_FUNC pfFunc, IN HANDLE hUserHandle)
{
    _KF_NODE_S *pstNode;
    _KF_CTRL_S *pstCtrl;

    pstCtrl = hKf;

    if (NULL == pstCtrl)
    {
        return BS_NULL_PARA;
    }

    pstNode = MEM_ZMalloc(sizeof(_KF_NODE_S));
    if (NULL == pstNode)
    {
        return BS_NO_MEMORY;
    }

    pstNode->pcKey = pcKey;
    pstNode->pfFunc = pfFunc;
    pstNode->hUserHandle = hUserHandle;

    DLL_ADD(&pstCtrl->stList, pstNode);

    return BS_OK;
}

BS_STATUS KF_RunMime(IN KF_HANDLE hKf, IN MIME_HANDLE hMime, IN HANDLE hRunHandle)
{
    CHAR *pcKey;
    _KF_NODE_S *pstNode;
    _KF_CTRL_S *pstCtrl = hKf;

    pcKey = MIME_GetKeyValue(hMime, pstCtrl->pcActionString);
    if (NULL == pcKey)
    {
        return BS_ERR;
    }

    pstNode = mf_Find(hKf, pcKey);
    if (NULL == pstNode)
    {
        return BS_ERR;
    }

    return pstNode->pfFunc(hMime, pstNode->hUserHandle, hRunHandle);
}

BS_STATUS KF_RunString(IN KF_HANDLE hKf, IN CHAR cSeparator, IN CHAR *pcString, IN HANDLE hRunHandle)
{
    MIME_HANDLE hMime;
    BS_STATUS eRet;
    
    hMime = MIME_Create();
    if (NULL == hMime)
    {
        return BS_NO_MEMORY;
    }

    if (BS_OK != MIME_Parse(hMime, cSeparator, pcString))
    {
        MIME_Destroy(hMime);
        return BS_ERR;
    }

    eRet = KF_RunMime(hKf, hMime, hRunHandle);

    MIME_Destroy(hMime);

    return eRet;
}


